use crate::word_enum;
use std::fmt;

use crate::error::{Error, Result};

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
        I8 => "i8",
        I16 => "i16",
        I32 => "i32",
        I64 => "i64",
        U8 => "u8",
        U16 => "u16",
        U32 => "u32",
        U64 => "u64",
        F16 => "f16",
        F32 => "f32",
        F64 => "f64",
        F128 => "f128",
        CI16 => "ci16",
        CI32 => "ci32",
        CI64 => "ci64",
        CI128 => "ci128",
        CU16 => "cu16",
        CU32 => "cu32",
        CU64 => "cu64",
        CU128 => "cu128",
        CF32 => "cf32",
        CF64 => "cf64",
        CF128 => "cf128",
        CF256 => "cf256",
    }
}

impl Prim {
    /// Indexed by the c enum prim_t value - also the serialized code
    pub const BY_CODE: [Prim; 24] = [
        Prim::U8,
        Prim::U16,
        Prim::U32,
        Prim::U64,
        Prim::I8,
        Prim::I16,
        Prim::I32,
        Prim::I64,
        Prim::F16,
        Prim::F32,
        Prim::F64,
        Prim::F128,
        Prim::CF32,
        Prim::CF64,
        Prim::CF128,
        Prim::CF256,
        Prim::CI16,
        Prim::CI32,
        Prim::CI64,
        Prim::CI128,
        Prim::CU16,
        Prim::CU32,
        Prim::CU64,
        Prim::CU128,
    ];

    /// The c enum prim_t value
    pub fn code(self) -> u8 {
        Self::BY_CODE
            .iter()
            .position(|&p| p == self)
            .expect("every prim is in BY_CODE") as u8
    }

    pub fn from_code(code: u32) -> Option<Prim> {
        Self::BY_CODE.get(code as usize).copied()
    }

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

impl<'a> Type<'a> {
    /// Fields are packed with no padding. Wraps on overflow
    pub fn byte_size(&self) -> u32 {
        match self {
            Type::Prim(p) => p.size_bytes() as u32,

            // Sum up all types
            Type::Struct(s) => s
                .types
                .iter()
                .fold(0u32, |acc, t| acc.wrapping_add(t.byte_size())),

            // Get the max size of each type
            Type::Union(u) => u.types.iter().map(Type::byte_size).max().unwrap_or(0),

            // Multiply each dimension
            Type::SArray(sa) => sa
                .dims
                .iter()
                .fold(1u32, |acc, &d| acc.wrapping_mul(d))
                .wrapping_mul(sa.t.byte_size()),
        }
    }

    pub fn validate(&self) -> Result<()> {
        match self {
            Type::Prim(_) => {}

            // Struct and union have the same validation
            Type::Struct(s) => validate_fields("Struct", &s.keys, &s.types)?,
            Type::Union(u) => validate_fields("Union", &u.keys, &u.types)?,

            // Dimensions > 0
            Type::SArray(sa) => {
                if sa.dims.is_empty() {
                    return Err(invalid("Strict Array: Rank must be > 0"));
                }
                if sa.dims.contains(&0) {
                    return Err(invalid("Strict Array: dimensions cannot be 0"));
                }

                // Validate subtype
                sa.t.validate()?;
            }
        }

        //
        self.checked_byte_size()?;
        self.serial_size().map(|_| ())
    }

    /// byte_size that fails instead of wrapping
    fn checked_byte_size(&self) -> Result<u32> {
        let too_big = || Error::Arith(format!("type is wider than {} bytes", u32::MAX));

        match self {
            Type::Prim(p) => Ok(p.size_bytes() as u32),

            Type::Struct(s) => s.types.iter().try_fold(0u32, |acc, t| {
                acc.checked_add(t.checked_byte_size()?).ok_or_else(too_big)
            }),

            Type::Union(u) => u
                .types
                .iter()
                .try_fold(0u32, |acc, t| Ok(acc.max(t.checked_byte_size()?))),

            Type::SArray(sa) => sa
                .dims
                .iter()
                .try_fold(sa.t.checked_byte_size()?, |acc, &d| {
                    acc.checked_mul(d).ok_or_else(too_big)
                }),
        }
    }

    pub fn serial_size(&self) -> Result<u16> {
        // TYPE
        let mut size: u16 = 1;
        let mut add = |n: usize| -> Result<()> {
            size = u16::try_from(n)
                .ok()
                .and_then(|n| size.checked_add(n))
                .ok_or_else(|| Error::Arith("serialized type is larger than 65535 bytes".into()))?;
            Ok(())
        };

        match self {
            // PRIM
            Type::Prim(_) => add(1)?,

            // LEN (KLEN KEY TYPE) (KLEN KEY TYPE) ...
            Type::Struct(Struct { keys, types }) | Type::Union(Union { keys, types }) => {
                add(2)?;
                for (key, t) in keys.iter().zip(types.iter()) {
                    add(2)?;
                    add(key.len())?;
                    add(t.serial_size()?.into())?;
                }
            }

            // RANK DIM0 DIM1 ... TYPE
            Type::SArray(sa) => {
                add(2)?;
                add(sa.dims.len().saturating_mul(4))?;
                add(sa.t.serial_size()?.into())?;
            }
        }

        Ok(size)
    }

    /// Integers are written in native byte order
    pub fn serialize(&self, out: &mut Vec<u8>) {
        match self {
            Type::Prim(p) => {
                out.push(TAG_PRIM);
                out.push(p.code());
            }
            Type::Struct(Struct { keys, types }) | Type::Union(Union { keys, types }) => {
                out.push(if matches!(self, Type::Struct(_)) {
                    TAG_STRUCT
                } else {
                    TAG_UNION
                });
                out.extend_from_slice(&(keys.len() as u16).to_ne_bytes());
                for (key, t) in keys.iter().zip(types.iter()) {
                    out.extend_from_slice(&(key.len() as u16).to_ne_bytes());
                    out.extend_from_slice(key.as_bytes());
                    t.serialize(out);
                }
            }
            Type::SArray(sa) => {
                out.push(TAG_SARRAY);
                out.extend_from_slice(&(sa.dims.len() as u16).to_ne_bytes());
                for dim in sa.dims.iter() {
                    out.extend_from_slice(&dim.to_ne_bytes());
                }
                sa.t.serialize(out);
            }
        }
    }

    /// Returns the type and the number of bytes read from [src]
    pub fn deserialize(src: &'a [u8]) -> Result<(Type<'a>, usize)> {
        let mut r = Reader { src, pos: 0 };
        let t = r.read_type()?;
        Ok((t, r.pos))
    }
}

const TAG_PRIM: u8 = 0;
const TAG_STRUCT: u8 = 1;
const TAG_UNION: u8 = 2;
const TAG_SARRAY: u8 = 3;

fn invalid(msg: impl Into<String>) -> Error {
    Error::InvalidType(msg.into())
}

fn validate_fields(what: &str, keys: &[&str], types: &[Type<'_>]) -> Result<()> {
    if keys.is_empty() || keys.len() != types.len() {
        return Err(invalid(format!("{what}: Keys length must be > 0")));
    }
    for (i, key) in keys.iter().enumerate() {
        if key.is_empty() {
            return Err(invalid(format!("{what}: Key length must be > 0")));
        }
        if keys[..i].contains(key) {
            return Err(invalid(format!("{what}: Duplicate keys")));
        }
    }
    types.iter().try_for_each(Type::validate)
}

struct Reader<'a> {
    src: &'a [u8],
    pos: usize,
}

impl<'a> Reader<'a> {
    fn bytes(&mut self, n: usize, what: &str) -> Result<&'a [u8]> {
        let end = self.pos.checked_add(n).filter(|&e| e <= self.src.len());
        let Some(end) = end else {
            return Err(Error::Corrupt(format!("Truncated type: missing {what}")));
        };
        let out = &self.src[self.pos..end];
        self.pos = end;
        Ok(out)
    }

    fn u16(&mut self, what: &str) -> Result<u16> {
        let b = self.bytes(2, what)?;
        Ok(u16::from_ne_bytes([b[0], b[1]]))
    }

    fn fields(&mut self, what: &str) -> Result<(Box<[&'a str]>, Box<[Type<'a>]>)> {
        let len = self.u16("field count")?;
        if len == 0 {
            return Err(Error::Corrupt(format!("{what}: no keys")));
        }

        let mut keys: Vec<&'a str> = Vec::new();
        let mut types = Vec::new();
        for _ in 0..len {
            let klen = self.u16("key length")?;
            let key = std::str::from_utf8(self.bytes(klen.into(), "key")?)
                .map_err(|_| Error::Corrupt(format!("{what}: key is not valid UTF-8")))?;
            if key.is_empty() {
                return Err(Error::Corrupt(format!("{what}: Key length must be > 0")));
            }
            if keys.contains(&key) {
                return Err(Error::Corrupt(format!("{what}: duplicate key: {key}")));
            }
            keys.push(key);
            types.push(self.read_type()?);
        }

        Ok((keys.into_boxed_slice(), types.into_boxed_slice()))
    }

    fn read_type(&mut self) -> Result<Type<'a>> {
        match self.bytes(1, "type code")?[0] {
            TAG_PRIM => {
                let code = self.bytes(1, "prim code")?[0];
                Prim::from_code(code.into())
                    .map(Type::Prim)
                    .ok_or_else(|| Error::Corrupt(format!("invalid prim type {code}")))
            }
            TAG_STRUCT => {
                let (keys, types) = self.fields("Struct")?;
                Ok(Type::Struct(Struct { keys, types }))
            }
            TAG_UNION => {
                let (keys, types) = self.fields("Union")?;
                Ok(Type::Union(Union { keys, types }))
            }
            TAG_SARRAY => {
                let rank = self.u16("rank")?;
                if rank == 0 {
                    return Err(Error::Corrupt("Strict Array: Rank must be > 0".into()));
                }
                let mut dims = Vec::new();
                for _ in 0..rank {
                    let b = self.bytes(4, "dimension")?;
                    let dim = u32::from_ne_bytes([b[0], b[1], b[2], b[3]]);
                    if dim == 0 {
                        return Err(Error::Corrupt(
                            "Strict Array: dimensions cannot be 0".into(),
                        ));
                    }
                    dims.push(dim);
                }
                let t = Box::new(self.read_type()?);
                Ok(Type::SArray(SArray {
                    dims: dims.into_boxed_slice(),
                    t,
                }))
            }
            other => Err(Error::Corrupt(format!("Unknown type code: {other}"))),
        }
    }
}

/// struct { a u32, b [10][20]f32 }
impl fmt::Display for Type<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Type::Prim(p) => write!(f, "{p}"),
            Type::Struct(Struct { keys, types }) | Type::Union(Union { keys, types }) => {
                f.write_str(if matches!(self, Type::Struct(_)) {
                    "struct { "
                } else {
                    "union { "
                })?;
                for (i, (key, t)) in keys.iter().zip(types.iter()).enumerate() {
                    if i > 0 {
                        f.write_str(", ")?;
                    }
                    write!(f, "{key} {t}")?;
                }
                f.write_str(" }")
            }
            Type::SArray(sa) => {
                for dim in sa.dims.iter() {
                    write!(f, "[{dim}]")?;
                }
                write!(f, "{}", sa.t)
            }
        }
    }
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

#[cfg(test)]
mod tests {
    use super::*;
    use crate::compiler::compile_type;

    const U8: Type<'static> = Type::Prim(Prim::U8);
    const U16: Type<'static> = Type::Prim(Prim::U16);
    const U32: Type<'static> = Type::Prim(Prim::U32);
    const CF128: Type<'static> = Type::Prim(Prim::CF128);

    fn serialized(t: &Type<'_>) -> Vec<u8> {
        let mut out = Vec::new();
        t.serialize(&mut out);
        out
    }

    fn u16b(v: u16) -> [u8; 2] {
        v.to_ne_bytes()
    }

    fn u32b(v: u32) -> [u8; 4] {
        v.to_ne_bytes()
    }

    // Serialized: foo u32, fo u8, baro u16, bazbi cf128
    fn four_fields(tag: u8) -> Vec<u8> {
        let mut exp = vec![tag];
        exp.extend(u16b(4));
        for (key, prim) in [
            ("foo", Prim::U32),
            ("fo", Prim::U8),
            ("baro", Prim::U16),
            ("bazbi", Prim::CF128),
        ] {
            exp.extend(u16b(key.len() as u16));
            exp.extend(key.as_bytes());
            exp.extend([TAG_PRIM, prim.code()]);
        }
        exp
    }

    fn four_field_list() -> Vec<(&'static str, Type<'static>)> {
        vec![("foo", U32), ("fo", U8), ("baro", U16), ("bazbi", CF128)]
    }

    /// Port of TEST (prim_t_validate)
    #[test]
    fn prim_codes_match_the_c_enum() {
        let names = [
            "u8", "u16", "u32", "u64", "i8", "i16", "i32", "i64", "f16", "f32", "f64", "f128",
            "cf32", "cf64", "cf128", "cf256", "ci16", "ci32", "ci64", "ci128", "cu16", "cu32",
            "cu64", "cu128",
        ];
        for (code, name) in names.iter().enumerate() {
            let p = Prim::from_code(code as u32).unwrap();
            assert_eq!(p.as_str(), *name);
            assert_eq!(usize::from(p.code()), code);
        }
        assert_eq!(Prim::from_code(24), None);
        assert_eq!(Prim::from_code(u32::MAX), None);
        assert_eq!(Prim::ALL.len(), Prim::BY_CODE.len());
    }

    /// Port of TEST (prim_t_byte_size)
    #[test]
    fn prim_byte_sizes() {
        use Prim::*;
        let cases = [
            (U8, 1),
            (I8, 1),
            (U16, 2),
            (I16, 2),
            (F16, 2),
            (CI16, 2),
            (CU16, 2),
            (U32, 4),
            (I32, 4),
            (F32, 4),
            (CF32, 4),
            (CI32, 4),
            (CU32, 4),
            (U64, 8),
            (I64, 8),
            (F64, 8),
            (CF64, 8),
            (CI64, 8),
            (CU64, 8),
            (F128, 16),
            (CF128, 16),
            (CI128, 16),
            (CU128, 16),
            (CF256, 32),
        ];
        assert_eq!(cases.len(), Prim::ALL.len());
        for (p, size) in cases {
            assert_eq!(Type::Prim(p).byte_size(), size, "{p}");
        }
    }

    /// Port of TEST (struct_t_byte_size), (union_t_byte_size), (sarray_t_byte_size)
    #[test]
    fn composite_byte_sizes() {
        // Fields are packed back to back
        assert_eq!(strct(four_field_list()).byte_size(), 4 + 1 + 2 + 16);
        // A union is as wide as its widest member
        assert_eq!(union(four_field_list()).byte_size(), 16);
        assert_eq!(sarray(&[10, 11, 12], U32).byte_size(), 10 * 11 * 12 * 4);
        assert_eq!(
            compile_type("[3]struct { a u8, b union { c u16, d [5]u64 } }")
                .unwrap()
                .byte_size(),
            3 * (1 + 40)
        );
    }

    #[test]
    fn byte_size_wraps_instead_of_panicking() {
        let huge = sarray(&[u32::MAX, u32::MAX], Type::Prim(Prim::U64));
        let _ = huge.byte_size();
    }

    // A type wider than u32::MAX bytes isn't valid
    #[test]
    fn validate_rejects_a_size_that_overflows() {
        let rejects = |src: &str| {
            let t = compile_type(src).unwrap();
            assert!(matches!(t.validate(), Err(Error::Arith(_))), "{src}");
        };

        // 2^32 bytes wraps to 0
        rejects("[65536][65536]u8");
        rejects("[65535][65535][65535]u64");
        rejects("struct { a [65535][65535]u8, b [65535][65535]u8 }");
        rejects("union { a u8, b [4]struct { c [65536][32768]u16 } }");

        // 2^32 - 1 bytes is the widest type
        let widest = compile_type("[65535][65537]u8").unwrap();
        assert!(widest.validate().is_ok());
        assert_eq!(widest.byte_size(), u32::MAX);
    }

    /// Port of TEST (prim_t_snprintf), (struct_t_snprintf), (union_t_snprintf),
    /// (sarray_t_snprintf)
    #[test]
    fn display() {
        for &p in Prim::ALL {
            assert_eq!(Type::Prim(p).to_string(), p.as_str());
        }
        assert_eq!(
            strct(four_field_list()).to_string(),
            "struct { foo u32, fo u8, baro u16, bazbi cf128 }"
        );
        assert_eq!(
            union(four_field_list()).to_string(),
            "union { foo u32, fo u8, baro u16, bazbi cf128 }"
        );
        assert_eq!(sarray(&[10, 11, 12], U32).to_string(), "[10][11][12]u32");
        assert_eq!(
            strct(vec![("a", sarray(&[2], strct(vec![("b", U8)])))]).to_string(),
            "struct { a [2]struct { b u8 } }"
        );
    }

    /// What is printed is what the parser accepts
    #[test]
    fn display_round_trips_through_the_parser() {
        for src in [
            "u8",
            "cf256",
            "[10][20]f32",
            "struct { a i32, b union { c i32, d [10][20]f32 } }",
            "[3]struct { x [2]union { y u8, z ci128 } }",
        ] {
            let t = compile_type(src).unwrap();
            assert_eq!(t.to_string(), src);
            assert_eq!(compile_type(&t.to_string()).unwrap(), t);
        }
    }

    /// Port of TEST (struct_t_validate_shallow), (union_t_validate_shallow),
    /// (sarray_t_validate_shallow) and (type_validate_rejects)
    #[test]
    fn validate() {
        assert!(U8.validate().is_ok());
        assert!(strct(four_field_list()).validate().is_ok());
        assert!(union(four_field_list()).validate().is_ok());
        assert!(sarray(&[10, 11, 12], U32).validate().is_ok());

        let rejects = |t: Type<'_>| {
            assert!(
                matches!(t.validate(), Err(Error::InvalidType(_))),
                "SHOULD FAIL: {t:?}"
            );
        };

        // No fields
        rejects(strct(vec![]));
        rejects(union(vec![]));
        // An empty key
        rejects(strct(vec![("", U8)]));
        rejects(union(vec![("a", U8), ("", U8)]));
        // Duplicate keys
        rejects(strct(vec![("a", U8), ("a", U16)]));
        rejects(union(vec![("a", U8), ("b", U16), ("a", U32)]));
        // Zero rank, zero dimension
        rejects(sarray(&[], U8));
        rejects(sarray(&[10, 0, 12], U8));
        // The same problems, nested
        rejects(strct(vec![("a", sarray(&[0], U8))]));
        rejects(sarray(&[2], union(vec![])));
        rejects(union(vec![("a", strct(vec![("b", U8), ("b", U8)]))]));

        // The same key in different structs is fine
        assert!(
            strct(vec![("a", U8), ("b", strct(vec![("a", U8)]))])
                .validate()
                .is_ok()
        );
    }

    /// Port of TEST (struct_t_get_serial_size), (union_t_get_serial_size),
    /// (sarray_t_get_serial_size)
    #[test]
    fn serial_size() {
        assert_eq!(U8.serial_size().unwrap(), 2);
        // TYPE LEN, then (KLEN KEY TYPE PRIM) per field
        let fields = 1 + 2 + (2 + 3 + 2) + (2 + 2 + 2) + (2 + 4 + 2) + (2 + 5 + 2);
        assert_eq!(strct(four_field_list()).serial_size().unwrap(), fields);
        assert_eq!(union(four_field_list()).serial_size().unwrap(), fields);
        // TYPE RANK DIM DIM DIM, then TYPE PRIM
        assert_eq!(
            sarray(&[10, 11, 12], U32).serial_size().unwrap(),
            1 + 2 + 3 * 4 + 2
        );

        for t in [
            U8,
            strct(four_field_list()),
            union(four_field_list()),
            sarray(&[10, 11, 12], strct(four_field_list())),
        ] {
            assert_eq!(usize::from(t.serial_size().unwrap()), serialized(&t).len());
        }
    }

    #[test]
    fn serial_size_overflow_is_an_error() {
        // One key that is too long on its own
        let key = "k".repeat(70_000);
        let t = strct(vec![(key.as_str(), U8)]);
        assert!(matches!(t.serial_size(), Err(Error::Arith(_))));
        assert!(matches!(t.validate(), Err(Error::Arith(_))));

        // More dimensions than fit
        let dims = vec![1u32; 20_000];
        assert!(matches!(
            sarray(&dims, U8).serial_size(),
            Err(Error::Arith(_))
        ));
    }

    /// Port of TEST (prim_t_serialize), (struct_t_serialize), (union_t_serialize),
    /// (sarray_t_serialize)
    #[test]
    fn serialize() {
        for &p in Prim::ALL {
            assert_eq!(serialized(&Type::Prim(p)), [TAG_PRIM, p.code()]);
        }

        assert_eq!(
            serialized(&strct(four_field_list())),
            four_fields(TAG_STRUCT)
        );
        assert_eq!(
            serialized(&union(four_field_list())),
            four_fields(TAG_UNION)
        );

        let mut exp = vec![TAG_SARRAY];
        exp.extend(u16b(3));
        exp.extend(u32b(10));
        exp.extend(u32b(11));
        exp.extend(u32b(12));
        exp.extend([TAG_PRIM, Prim::U32.code()]);
        assert_eq!(serialized(&sarray(&[10, 11, 12], U32)), exp);
    }

    /// Port of TEST (prim_t_deserialize), (struct_t_deserialize_green_path),
    /// (union_t_deserialize_green_path), (sarray_t_deserialize_green_path)
    #[test]
    fn deserialize() {
        for &p in Prim::ALL {
            let bytes = [TAG_PRIM, p.code()];
            assert_eq!(Type::deserialize(&bytes).unwrap(), (Type::Prim(p), 2));
        }

        let bytes = four_fields(TAG_STRUCT);
        assert_eq!(
            Type::deserialize(&bytes).unwrap(),
            (strct(four_field_list()), bytes.len())
        );

        let bytes = four_fields(TAG_UNION);
        assert_eq!(
            Type::deserialize(&bytes).unwrap(),
            (union(four_field_list()), bytes.len())
        );

        let t = sarray(&[10, 11, 12], U32);
        let bytes = serialized(&t);
        assert_eq!(Type::deserialize(&bytes).unwrap(), (t, bytes.len()));
    }

    #[test]
    fn deserialize_reads_one_type_and_stops() {
        let t = compile_type("struct { a [4]u16, b union { c f64, d i8 } }").unwrap();
        let mut bytes = serialized(&t);
        let len = bytes.len();
        bytes.extend([0xff; 7]);
        assert_eq!(Type::deserialize(&bytes).unwrap(), (t, len));
    }

    #[test]
    fn serialize_round_trips() {
        for src in [
            "i64",
            "[1]u8",
            "[10][20]f32",
            "struct { a i32, b union { c i32, d [10][20]f32 } }",
            "[3]struct { x [2]union { y u8, z ci128 } }",
            "union { a struct { b struct { c struct { d cf256 } } } }",
        ] {
            let t = compile_type(src).unwrap();
            let bytes = serialized(&t);
            assert_eq!(Type::deserialize(&bytes).unwrap(), (t, bytes.len()));
        }
    }

    /// Port of TEST (type_deserialize_rejects), (struct_t_deserialize_red_path),
    /// (union_t_deserialize_red_path), (sarray_t_deserialize_red_path)
    #[test]
    fn deserialize_rejects() {
        let rejects = |bytes: &[u8]| {
            assert!(
                matches!(Type::deserialize(bytes), Err(Error::Corrupt(_))),
                "SHOULD FAIL: {bytes:?}"
            );
        };

        // Nothing at all, an unknown type code, an unknown prim code
        rejects(&[]);
        rejects(&[99]);
        rejects(&[TAG_PRIM, 24]);
        rejects(&[TAG_PRIM, 0xff]);

        for tag in [TAG_STRUCT, TAG_UNION] {
            // No fields
            let mut bytes = vec![tag];
            bytes.extend(u16b(0));
            rejects(&bytes);

            // A duplicate key
            let mut bytes = vec![tag];
            bytes.extend(u16b(2));
            for _ in 0..2 {
                bytes.extend(u16b(3));
                bytes.extend(b"foo");
                bytes.extend([TAG_PRIM, Prim::U8.code()]);
            }
            rejects(&bytes);

            // An empty key
            let mut bytes = vec![tag];
            bytes.extend(u16b(1));
            bytes.extend(u16b(0));
            bytes.extend([TAG_PRIM, Prim::U8.code()]);
            rejects(&bytes);

            // A key that is not text
            let mut bytes = vec![tag];
            bytes.extend(u16b(1));
            bytes.extend(u16b(2));
            bytes.extend([0xff, 0xfe]);
            bytes.extend([TAG_PRIM, Prim::U8.code()]);
            rejects(&bytes);

            // A bad field type
            let mut bytes = vec![tag];
            bytes.extend(u16b(1));
            bytes.extend(u16b(1));
            bytes.extend(b"a");
            bytes.extend([TAG_PRIM, 200]);
            rejects(&bytes);
        }

        // Zero rank
        let mut bytes = vec![TAG_SARRAY];
        bytes.extend(u16b(0));
        bytes.extend([TAG_PRIM, Prim::U32.code()]);
        rejects(&bytes);

        // A zero dimension: [10][0][12]u32
        let mut bytes = vec![TAG_SARRAY];
        bytes.extend(u16b(3));
        bytes.extend(u32b(10));
        bytes.extend(u32b(0));
        bytes.extend(u32b(12));
        bytes.extend([TAG_PRIM, Prim::U32.code()]);
        rejects(&bytes);
    }

    /// Port of TEST (struct_t_deserialize_early_termination),
    /// (union_t_deserialize_early_termination),
    /// (sarray_t_deserialize_early_termination)
    #[test]
    fn deserialize_early_termination() {
        for t in [
            U32,
            strct(four_field_list()),
            union(four_field_list()),
            sarray(&[10, 11, 12], U32),
            sarray(&[2], strct(vec![("a", union(four_field_list()))])),
        ] {
            let bytes = serialized(&t);
            for cut in 0..bytes.len() {
                assert!(
                    matches!(Type::deserialize(&bytes[..cut]), Err(Error::Corrupt(_))),
                    "{t} cut at {cut}"
                );
            }
        }
    }

    /// Port of TEST (struct_t_equal), (union_t_equal), (sarray_t_equal)
    #[test]
    fn equality() {
        assert_eq!(strct(four_field_list()), strct(four_field_list()));
        assert_ne!(strct(four_field_list()), union(four_field_list()));

        // A different key, a different type, a different length
        assert_ne!(strct(vec![("a", U8)]), strct(vec![("b", U8)]));
        assert_ne!(strct(vec![("a", U8)]), strct(vec![("a", U16)]));
        assert_ne!(strct(vec![("a", U8)]), strct(vec![("a", U8), ("b", U8)]));
        assert_ne!(union(vec![("a", U8)]), union(vec![("a", U16)]));

        assert_eq!(sarray(&[1, 2], U8), sarray(&[1, 2], U8));
        assert_ne!(sarray(&[1, 2], U8), sarray(&[2, 1], U8));
        assert_ne!(sarray(&[1, 2], U8), sarray(&[1], U8));
        assert_ne!(sarray(&[1, 2], U8), sarray(&[1, 2], U16));
        assert_ne!(sarray(&[1], U8), U8);
    }
}
