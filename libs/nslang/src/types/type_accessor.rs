use std::num::NonZeroI64;

use crate::error::{Error, Result};
use crate::types::types::{SArray, Type};
use crate::types::user_stride::UserStride;

#[derive(Debug, Clone, PartialEq)]
pub enum TypeAccessor<'src> {
    /// The type as-is. Ends every chain.
    Take,

    /// A field of a struct or union
    Select {
        key: &'src str,
        sub: Box<TypeAccessor<'src>>,
    },

    /// Index or slice the leading dimensions of an sarray
    Range {
        strides: Box<[UserStride]>,
        sub: Box<TypeAccessor<'src>>,
    },
}

impl<'src> TypeAccessor<'src> {
    pub fn subtype(&self, reftype: &Type<'src>) -> Result<Type<'src>> {
        match self {
            TypeAccessor::Take => Ok(reftype.clone()),

            TypeAccessor::Select { key, sub } => {
                let (keys, types) = match reftype {
                    // Select on an array broadcasts:
                    // a.i on [10]struct { i i32 } is [10]i32
                    Type::SArray(sa) => {
                        return Ok(wrap_dims(sa.dims.to_vec(), self.subtype(&sa.t)?));
                    }
                    Type::Struct(s) => (&s.keys, &s.types),
                    Type::Union(u) => (&u.keys, &u.types),
                    _ => {
                        return Err(ta_error(format!(
                            "Cannot select '{key}' from a non struct/union type"
                        )));
                    }
                };
                let i = keys
                    .iter()
                    .position(|k| k == key)
                    .ok_or_else(|| ta_error(format!("No field named '{key}'")))?;
                sub.subtype(&types[i])
            }

            TypeAccessor::Range { strides, sub } => {
                let Type::SArray(sa) = reftype else {
                    return Err(ta_error("Cannot index a non-array type"));
                };
                if strides.len() > sa.dims.len() {
                    return Err(ta_error(format!(
                        "{} indices given for an array with {} dimensions",
                        strides.len(),
                        sa.dims.len()
                    )));
                }

                // Sliced dims survive with their new length; indexed dims drop out
                let mut dims = Vec::new();
                for (stride, &len) in strides.iter().zip(sa.dims.iter()) {
                    if let Some(n) = apply_stride(stride, len)? {
                        dims.push(n);
                    }
                }

                // Untouched trailing dims stay attached to the element, so
                // `sub` sees e.g. `[20]f32` after `[i]` on a `[10][20]f32`
                let rest = &sa.dims[strides.len()..];
                let inner = if rest.is_empty() {
                    (*sa.t).clone()
                } else {
                    Type::SArray(SArray {
                        dims: rest.into(),
                        t: sa.t.clone(),
                    })
                };

                Ok(wrap_dims(dims, sub.subtype(&inner)?))
            }
        }
    }
}

fn wrap_dims<'src>(mut dims: Vec<u32>, inner: Type<'src>) -> Type<'src> {
    if dims.is_empty() {
        return inner;
    }
    match inner {
        Type::SArray(sa) => {
            dims.extend_from_slice(&sa.dims);
            Type::SArray(SArray {
                dims: dims.into_boxed_slice(),
                t: sa.t,
            })
        }
        t => Type::SArray(SArray {
            dims: dims.into_boxed_slice(),
            t: Box::new(t),
        }),
    }
}

fn apply_stride(stride: &UserStride, len: u32) -> Result<Option<u32>> {
    let len = i64::from(len);

    match *stride {
        UserStride::Index(i) => {
            let adj = if i < 0 { i + len } else { i };
            if !(0..len).contains(&adj) {
                return Err(ta_error(format!(
                    "Index {i} out of bounds for dimension of size {len}"
                )));
            }
            Ok(None)
        }

        UserStride::Slice { start, stop, step } => {
            let step = step.map_or(1, NonZeroI64::get);

            // Negative counts from the end, then clamp into range
            let clamp = |v: i64| {
                let v = if v < 0 { v + len } else { v };
                if step > 0 {
                    v.clamp(0, len)
                } else {
                    v.clamp(-1, len - 1)
                }
            };

            let (start, stop) = if step > 0 {
                (start.map_or(0, clamp), stop.map_or(len, clamp))
            } else {
                (start.map_or(len - 1, clamp), stop.map_or(-1, clamp))
            };

            let n = if step > 0 && start < stop {
                (stop - start - 1) / step + 1
            } else if step < 0 && stop < start {
                (start - stop - 1) / -step + 1
            } else {
                0
            };

            if n == 0 {
                return Err(ta_error("Slice selects no elements"));
            }
            Ok(Some(n as u32))
        }
    }
}

fn ta_error(msg: impl Into<String>) -> Error {
    Error::InvalidType(msg.into())
}

pub fn take<'src>() -> TypeAccessor<'src> {
    TypeAccessor::Take
}

pub fn select<'src>(key: &'src str, sub: TypeAccessor<'src>) -> TypeAccessor<'src> {
    TypeAccessor::Select {
        key,
        sub: Box::new(sub),
    }
}

pub fn range<'src>(strides: Vec<UserStride>, sub: TypeAccessor<'src>) -> TypeAccessor<'src> {
    TypeAccessor::Range {
        strides: strides.into_boxed_slice(),
        sub: Box::new(sub),
    }
}

#[cfg(test)]
mod tests {
    use crate::compiler::{compile_subtype, compile_type};

    /// Port of TEST (ta_subtype). The rules being exercised:
    ///   * field select on an aggregate picks the field's type
    ///   * field select on an ARRAY of aggregates BROADCASTS: the field type
    ///     is re-wrapped in the outer array dimensions
    ///   * a single index removes one dimension
    ///   * a stride s:e:st resizes a dimension to ceil((e-s)/st)
    #[test]
    fn subtype() {
        let entries = [
            // field select: baseline + non-first field
            ("struct { i i32 }", "a.i", "i32"),
            ("struct { x u32, y f64 }", "a.y", "f64"),
            ("union { x f32, y f64 }", "a.y", "f64"),
            // nested aggregates
            (
                "struct { a struct { b struct { c f32 } } }",
                "x.a.b.c",
                "f32",
            ),
            ("struct { a struct { b i32, c f64 }, d i8 }", "x.a.c", "f64"),
            ("union { a union { b i32, c f64 }, d i8 }", "x.a.c", "f64"),
            // 1D single index: dimension removal at the boundaries
            ("[10] i32", "a[0]", "i32"),
            ("[10] u32", "a[9]", "u32"),
            // 1D stride: new size = ceil((stop - start) / step)
            ("[10] i32", "a[0:10:1]", "[10] i32"),
            ("[10] i32", "a[0:10:3]", "[4] i32"),
            ("[20] f32", "a[2:10:2]", "[4] f32"),
            ("[10] i32", "a[0:1:1]", "[1] i32"),
            // multi-dim: full collapse / full stride / mixed removal
            ("[2][3][4] f32", "a[0, 1, 2]", "f32"),
            ("[6][8][10] i32", "a[0:6:2, 0:8:4, 0:10:5]", "[3][2][2] i32"),
            ("[2][3][4] f32", "a[0:2:1, 1, 0:4:2]", "[2][2] f32"),
            ("[4][6][8] i64", "a[2, 0:6:3, 3]", "[2] i64"),
            // struct/union containing an array
            ("struct { data [100] f64 }", "a.data[5]", "f64"),
            (
                "struct { data [10][20] i32 }",
                "a.data[0:10:5, 3]",
                "[2] i32",
            ),
            (
                "union { data [10][20] i32 }",
                "a.data[0:10:5, 3]",
                "[2] i32",
            ),
            // field select broadcasts over array dims
            ("[10] struct { i i32 }", "a.i", "[10] i32"),
            ("[10] struct { x f32, y f64 }", "a.y", "[10] f64"),
            ("[3][4] struct { v f32 }", "a.v", "[3][4] f32"),
            ("[5] struct { d [10] i32 }", "a.d", "[5][10] i32"),
            ("[6] struct { p struct { q i16 } }", "a.p.q", "[6] i16"),
            ("[10] union { x f32, y f32 }", "a.y", "[10] f32"),
            // index/stride on array-of-aggregate, THEN field
            ("[10] struct { x f32, y f32 }", "a[3].x", "f32"),
            ("[10] struct { x f32, y f32 }", "a[0:10:2].y", "[5] f32"),
            ("[10] union { x f32, y f32 }", "a[0:10:2].y", "[5] f32"),
            // struct -> array -> struct chains
            (
                "struct { points [100] struct { val f32 } }",
                "a.points[7].val",
                "f32",
            ),
            (
                "struct { points [100] struct { val f32 } }",
                "a.points[0:50:2].val",
                "[25] f32",
            ),
            // deep nest + array stride broadcast
            (
                "struct { a struct { b [20] struct { c i32 } } }",
                "a.a.b[0:20:4].c",
                "[5] i32",
            ),
            // sibling fields don't shift the array-field result
            (
                "struct { a i32, b f64, c [10] i8 }",
                "a.c[0:10:2]",
                "[5] i8",
            ),
            // Fewer indices than dimensions: the rest stay
            ("[2][3][4] u8", "a[1]", "[3][4] u8"),
            ("[2][3][4] u8", "a[:, 0]", "[2][4] u8"),
            // Negative indices count from the end, open ends are the edges
            ("[10] u8", "a[-1]", "u8"),
            ("[10] u8", "a[:]", "[10] u8"),
            ("[10] u8", "a[5:]", "[5] u8"),
            ("[10] u8", "a[:-2]", "[8] u8"),
            ("[10] u8", "a[::-1]", "[10] u8"),
            ("[10] u8", "a[8:2:-2]", "[3] u8"),
            // No accessor at all
            ("struct { a u8 }", "a", "struct { a u8 }"),
        ];

        for (typestr, accessor, expected) in entries {
            let reftype = compile_type(typestr).unwrap();
            let st = compile_subtype(accessor).unwrap();
            let expected = compile_type(expected).unwrap();
            assert_eq!(
                st.get_type(&reftype).unwrap(),
                expected,
                "{typestr} :: {accessor}"
            );
        }
    }

    /// Port of TEST (ta_subtype_rejects)
    #[test]
    fn subtype_rejects() {
        let entries = [
            ("struct { x f32 }", "a.z"),
            ("union { x f32 }", "a.z"),
            ("u8", "a.x"),
            ("struct { x u8 }", "a[0]"),
            ("[10] struct { x f32 }", "a.z"),
            ("[10] struct { x f32 }", "a[0:5].z"),
            ("[10] struct { x f32 }", "a[0].z"),
            // Out of bounds, too many indices, nothing selected
            ("[10] u8", "a[10]"),
            ("[10] u8", "a[-11]"),
            ("[10] u8", "a[0, 0]"),
            ("[10] u8", "a[5:5]"),
            ("u8", "a[0]"),
        ];

        for (typestr, accessor) in entries {
            let reftype = compile_type(typestr).unwrap();
            let st = compile_subtype(accessor).unwrap();
            assert!(
                st.get_type(&reftype).is_err(),
                "SHOULD FAIL: {typestr} :: {accessor}"
            );
        }
    }
}
