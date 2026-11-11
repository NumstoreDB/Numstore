use crate::error::Result;
use crate::types::sub_type::Subtype;
use crate::types::type_accessor::TypeAccessor;
use crate::types::types::{Struct, Type};

#[derive(Debug, Clone, PartialEq)]
pub enum TypeRef<'src> {
    /// `a.b`: the type reached by walking `ta` into variable `vname`
    Take(Subtype<'src>),

    /// `struct { f a.a, g a.b }`: a new struct whose fields are type refs
    Struct(StructRef<'src>),
}

#[derive(Debug, Clone, PartialEq)]
pub struct StructRef<'src> {
    pub keys: Box<[&'src str]>,
    pub types: Box<[TypeRef<'src>]>,
}

impl<'src> TypeRef<'src> {
    pub fn construct(&self, reftype: &Type<'src>) -> Result<Type<'src>> {
        match self {
            TypeRef::Take(take) => take.ta.subtype(reftype),

            TypeRef::Struct(s) => {
                let types = s
                    .types
                    .iter()
                    .map(|t| t.construct(reftype))
                    .collect::<Result<Box<[_]>>>()?;

                Ok(Type::Struct(Struct {
                    keys: s.keys.clone(),
                    types,
                }))
            }
        }
    }
}

pub fn take<'src>(vname: &'src str, ta: TypeAccessor<'src>) -> TypeRef<'src> {
    TypeRef::Take(Subtype { vname, ta })
}

pub fn strct_ref<'src>(fields: Vec<(&'src str, TypeRef<'src>)>) -> TypeRef<'src> {
    let (keys, types): (Vec<_>, Vec<_>) = fields.into_iter().unzip();
    TypeRef::Struct(StructRef {
        keys: keys.into_boxed_slice(),
        types: types.into_boxed_slice(),
    })
}

#[cfg(test)]
mod tests {
    use crate::compiler::{compile_type, compile_type_ref};

    #[test]
    fn construct() {
        let green = [
            ("u8", "foo", "u8"),
            ("struct { a u8 }", "foo", "struct { a u8 }"),
            ("struct { a u8 }", "foo.a", "u8"),
            (
                "struct { a u8, b struct { c u16 } }",
                "foo.b",
                "struct { c u16 }",
            ),
            ("struct { a u8, b struct { c u16 } }", "foo.b.c", "u16"),
            ("struct { a u8, b [10][20]u16 }", "foo.b", "[10][20]u16"),
            (
                "struct { a u8, b struct { c u16 } }",
                "struct { a foo.b, b foo.a }",
                "struct { a struct { c u16 }, b u8 }",
            ),
            (
                "struct { a u8, b [10]u16 }",
                "struct { x struct { y foo.b[0:4] }, z foo.a }",
                "struct { x struct { y [4]u16 }, z u8 }",
            ),
        ];
        for (typestr, trstr, expected) in green {
            let base = compile_type(typestr).unwrap();
            let tr = compile_type_ref(trstr).unwrap();
            assert_eq!(
                tr.construct(&base).unwrap(),
                compile_type(expected).unwrap(),
                "tr_construct({typestr}, {trstr})"
            );
        }

        let red = [
            ("u8", "foo.bar"),
            ("struct { a u8 }", "foo.bar"),
            ("struct { a u8 }", "foo.a.b"),
            ("struct { a u8 }", "struct { x foo.a, y foo.nope }"),
        ];
        for (typestr, trstr) in red {
            let base = compile_type(typestr).unwrap();
            let tr = compile_type_ref(trstr).unwrap();
            assert!(
                tr.construct(&base).is_err(),
                "tr_construct({typestr}, {trstr}) == FAIL"
            );
        }
    }
}
