// Random types for tests and the simulation

use crate::compiler::is_keyword;
use crate::error::{Error, Result};
use crate::types::types::{Prim, SArray, Struct, Type, Union};

/// A type that owns its keys. A random type has no source text to borrow from
#[derive(Debug, Clone, PartialEq)]
pub enum OwnedType {
    Prim(Prim),
    Struct(Vec<(String, OwnedType)>),
    Union(Vec<(String, OwnedType)>),
    SArray(Vec<u32>, Box<OwnedType>),
}

impl OwnedType {
    pub fn view(&self) -> Type<'_> {
        fn fields(src: &[(String, OwnedType)]) -> (Box<[&str]>, Box<[Type<'_>]>) {
            (
                src.iter().map(|(k, _)| k.as_str()).collect(),
                src.iter().map(|(_, t)| t.view()).collect(),
            )
        }

        match self {
            OwnedType::Prim(p) => Type::Prim(*p),
            OwnedType::Struct(f) => {
                let (keys, types) = fields(f);
                Type::Struct(Struct { keys, types })
            }
            OwnedType::Union(f) => {
                let (keys, types) = fields(f);
                Type::Union(Union { keys, types })
            }
            OwnedType::SArray(dims, t) => Type::SArray(SArray {
                dims: dims.as_slice().into(),
                t: Box::new(t.view()),
            }),
        }
    }
}

/// rng(lo, hi) returns a number in lo..=hi
pub type Rng<'r> = &'r mut dyn FnMut(u32, u32) -> u32;

/// 100 is arbitrary - just an upper bound
const ATTEMPTS: u32 = 100;

/// A valid type at most [depth] levels deep and [max_size] bytes wide
pub fn random_type(rng: Rng<'_>, depth: u32, max_size: u32) -> Result<OwnedType> {
    for _ in 0..ATTEMPTS {
        let Ok(t) = random_once(rng, depth, max_size) else {
            continue;
        };
        let view = t.view();
        if view.validate().is_ok() && view.byte_size() <= max_size {
            return Ok(t);
        }
    }

    Err(Error::InvalidType(format!(
        "Failed to generate a valid type of at most {max_size} bytes"
    )))
}

fn random_prim(rng: Rng<'_>) -> Prim {
    let last = Prim::BY_CODE.len() as u32 - 1;
    Prim::BY_CODE[rng(0, last).min(last) as usize]
}

fn random_name(rng: Rng<'_>) -> String {
    loop {
        let len = rng(5, 11).clamp(1, 11);
        let name: String = (0..len)
            .map(|_| char::from(b'a' + rng(0, 25).min(25) as u8))
            .collect();
        if !is_keyword(&name) {
            return name;
        }
    }
}

fn random_fields(rng: Rng<'_>, depth: u32, max_size: u32) -> Result<Vec<(String, OwnedType)>> {
    let len = rng(1, 5).clamp(1, 5);
    (0..len)
        .map(|_| Ok((random_name(rng), random_type(rng, depth - 1, max_size)?)))
        .collect()
}

fn random_once(rng: Rng<'_>, depth: u32, max_size: u32) -> Result<OwnedType> {
    if depth == 0 {
        return Ok(OwnedType::Prim(random_prim(rng)));
    }

    match rng(0, 3) {
        0 => Ok(OwnedType::Prim(random_prim(rng))),
        1 => Ok(OwnedType::Struct(random_fields(rng, depth, max_size)?)),
        2 => Ok(OwnedType::Union(random_fields(rng, depth, max_size)?)),
        _ => {
            let rank = rng(1, 4).clamp(1, 4);
            let mut dims: Vec<u32> = (0..rank).map(|_| rng(1, 11).max(1)).collect();
            // Flatten an array of arrays like the parser does
            match random_type(rng, depth - 1, max_size)? {
                OwnedType::SArray(inner, t) => {
                    dims.extend(inner);
                    Ok(OwnedType::SArray(dims, t))
                }
                t => Ok(OwnedType::SArray(dims, Box::new(t))),
            }
        }
    }
}

#[cfg(test)]
pub(crate) mod tests {
    use super::*;

    // xorshift
    pub(crate) fn test_rng(seed: u64) -> impl FnMut(u32, u32) -> u32 {
        let mut state = seed | 1;
        move |lo, hi| {
            state ^= state << 13;
            state ^= state >> 7;
            state ^= state << 17;
            lo + (state % u64::from(hi - lo + 1)) as u32
        }
    }

    /// Port of TEST (type_random)
    #[test]
    fn random_types_are_valid_and_small_enough() {
        let mut rng = test_rng(0x5eed);
        for _ in 0..200 {
            let t = random_type(&mut rng, 3, 4096).unwrap();
            let view = t.view();
            assert!(view.validate().is_ok(), "{view}");
            assert!(view.byte_size() <= 4096, "{view}");
        }
    }

    /// Port of TEST (prim_t_random): depth 0 is always a primitive
    #[test]
    fn depth_zero_is_a_primitive() {
        let mut rng = test_rng(7);
        for _ in 0..100 {
            assert!(matches!(
                random_type(&mut rng, 0, 64).unwrap(),
                OwnedType::Prim(_)
            ));
        }
    }

    /// Port of TEST (type_random_gives_up): nothing is 0 bytes wide
    #[test]
    fn gives_up_when_nothing_fits() {
        let mut rng = test_rng(99);
        assert!(matches!(
            random_type(&mut rng, 2, 0),
            Err(Error::InvalidType(_))
        ));
    }

    #[test]
    fn random_names_are_usable() {
        let mut rng = test_rng(3);
        for _ in 0..1000 {
            let name = random_name(&mut rng);
            assert!((5..=11).contains(&name.len()));
            assert!(!is_keyword(&name));
            assert!(crate::compiler::compile_subtype(&name).is_ok());
        }
    }

    #[test]
    fn random_types_round_trip_through_text() {
        let mut rng = test_rng(11);
        for _ in 0..100 {
            let t = random_type(&mut rng, 3, 1 << 20).unwrap();
            let text = t.view().to_string();
            assert_eq!(crate::compiler::compile_type(&text).unwrap(), t.view());
        }
    }
}
