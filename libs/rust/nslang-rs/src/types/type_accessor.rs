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
