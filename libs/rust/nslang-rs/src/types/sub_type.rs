use crate::error::Result;
use crate::types::type_accessor::TypeAccessor;
use crate::types::types::Type;

#[derive(Debug, Clone, PartialEq)]
pub struct Subtype<'src> {
    pub vname: &'src str,
    pub ta: TypeAccessor<'src>,
}

impl<'src> Subtype<'src> {
    pub fn new(vname: &'src str, ta: TypeAccessor<'src>) -> Self {
        Subtype { vname, ta }
    }

    pub fn get_type(&self, stype: &Type<'src>) -> Result<Type<'src>> {
        self.ta.subtype(stype)
    }
}
