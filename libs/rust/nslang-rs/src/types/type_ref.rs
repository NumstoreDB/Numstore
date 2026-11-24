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
