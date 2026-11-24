use std::fmt;
use std::str::FromStr;

#[repr(u8)]
#[derive(Debug, Clone, PartialEq)]
pub enum Type<'a> {
    Prim(Prim) = 0,
    Struct(Struct<'a>) = 1,
    Union(Union<'a>) = 2,
    SArray(SArray<'a>) = 3,
}

word_enum! {
    /// Primitive element types. The number in each name is its size in bits.
    Prim {
        I8 => "i8", I16 => "i16", I32 => "i32", I64 => "i64",
        U8 => "u8", U16 => "u16", U32 => "u32", U64 => "u64",
        F16 => "f16", F32 => "f32", F64 => "f64", F128 => "f128",
        CI16 => "ci16", CI32 => "ci32", CI64 => "ci64", CI128 => "ci128",
        CU16 => "cu16", CU32 => "cu32", CU64 => "cu64", CU128 => "cu128",
        CF32 => "cf32", CF64 => "cf64", CF128 => "cf128", CF256 => "cf256",
    }
}

impl Prim {
    pub fn size_bytes(self) -> usize {
        let bits: usize = self
            .as_str()
            .trim_start_matches(|c: char| c.is_ascii_alphabetic())
            .parse()
            .expect("prim names end in their bit width");
        bits / 8
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct Struct<'a> {
    pub keys: Box<[&'a str]>,
    pub types: Box<[Type<'a>]>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct Union<'a> {
    pub keys: Box<[&'a str]>,
    pub types: Box<[Type<'a>]>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct SArray<'a> {
    pub dims: Box<[u32]>,
    pub t: Box<Type<'a>>,
}

impl Struct<'_> {
    pub fn len(&self) -> u16 {
        self.keys.len() as u16
    }
}

impl Union<'_> {
    pub fn len(&self) -> u16 {
        self.keys.len() as u16
    }
}

impl SArray<'_> {
    pub fn rank(&self) -> u16 {
        self.dims.len() as u16
    }
}

///// Easy constructors
pub fn sarray<'a>(dims: &[u32], t: Type<'a>) -> Type<'a> {
    Type::SArray(SArray {
        dims: dims.into(),
        t: Box::new(t),
    })
}

pub fn strct<'a>(fields: Vec<(&'a str, Type<'a>)>) -> Type<'a> {
    let (keys, types): (Vec<_>, Vec<_>) = fields.into_iter().unzip();
    Type::Struct(Struct {
        keys: keys.into(),
        types: types.into(),
    })
}

pub fn union<'a>(fields: Vec<(&'a str, Type<'a>)>) -> Type<'a> {
    let (keys, types): (Vec<_>, Vec<_>) = fields.into_iter().unzip();
    Type::Union(Union {
        keys: keys.into(),
        types: types.into(),
    })
}

pub const I32: Type<'static> = Type::Prim(Prim::I32);
pub const F32: Type<'static> = Type::Prim(Prim::F32);
pub const F64: Type<'static> = Type::Prim(Prim::F64);
