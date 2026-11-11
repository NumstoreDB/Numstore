//! Type grammar:
//!
//! ```text
//! type           ::= struct_type | union_type | sarray_type | primitive_type
//! primitive_type ::= PRIM
//! sarray_type    ::= ( '[' INTEGER ']' )+ type
//! struct_type    ::= 'struct' '{' field ( ',' field )* '}'
//! union_type     ::= 'union'  '{' field ( ',' field )* '}'
//! field          ::= IDENT type
//! ```

use super::Parser;
use crate::error::Result;
use crate::tokens::{Keyword, TokenKind};
use crate::types::types::{SArray, Struct, Type, Union};

type Fields<'src> = (Box<[&'src str]>, Box<[Type<'src>]>);

impl<'src> Parser<'src> {
    /// type ::= struct_type | union_type | sarray_type | primitive_type
    pub fn parse_type(&mut self) -> Result<Type<'src>> {
        match self.peek_kind() {
            // struct ...
            TokenKind::Keyword(Keyword::Struct) => {
                let (keys, types) = self.parse_fields(TokenKind::Keyword(Keyword::Struct))?;
                Ok(Type::Struct(Struct { keys, types }))
            }

            // union ...
            TokenKind::Keyword(Keyword::Union) => {
                let (keys, types) = self.parse_fields(TokenKind::Keyword(Keyword::Union))?;
                Ok(Type::Union(Union { keys, types }))
            }

            // [ ...
            TokenKind::LBracket => self.parse_sarray_type(),

            // u8 ...
            TokenKind::Prim(p) => {
                self.advance();
                Ok(Type::Prim(p))
            }

            other => Err(self.error(format!("Expected type, got {other:?}"))),
        }
    }

    /// sarray_type ::= ( '[' INTEGER ']' )+ type
    fn parse_sarray_type(&mut self) -> Result<Type<'src>> {
        let mut dims = Vec::new();

        // Parse all [N] brackets
        while self.match_tokens(TokenKind::LBracket) {
            dims.push(self.parse_dim()?);
            self.expect(TokenKind::RBracket)?;
        }

        // Inner most type
        let t = Box::new(self.parse_type()?);

        Ok(Type::SArray(SArray {
            dims: dims.into_boxed_slice(),
            t,
        }))
    }

    fn parse_dim(&mut self) -> Result<u32> {
        match self.maybe_parse_integer() {
            Some(v) => {
                if v == 0 {
                    Err(self.error("Cannot have dimension 0"))
                } else {
                    u32::try_from(v).map_err(|_| self.error("Failed to parse dim"))
                }
            }
            None => Err(self.error(format!("Expected dimension"))),
        }
    }

    /// ('struct' | 'union') '{' field ( ',' field )* '}'
    fn parse_fields(&mut self, keyword: TokenKind<'src>) -> Result<Fields<'src>> {
        // struct
        self.expect(keyword)?;

        // {
        self.expect(TokenKind::LBrace)?;

        let mut keys = Vec::new();
        let mut types = Vec::new();

        loop {
            // foo TYPE
            let (key, t) = self.parse_field()?;

            // Ensure no duplicate keys
            if keys.contains(&key) {
                return Err(self.error(format!("Duplicate field name '{key}'")));
            }

            keys.push(key);
            types.push(t);

            // foo TYPE }
            if self.peek_kind() != TokenKind::Comma {
                break;
            }

            // foo TYPE,
            self.advance();
        }

        // foo TYPE }
        self.expect(TokenKind::RBrace)?;

        Ok((keys.into_boxed_slice(), types.into_boxed_slice()))
    }

    /// field ::= IDENT type
    fn parse_field(&mut self) -> Result<(&'src str, Type<'src>)> {
        let key = match self.peek_kind() {
            TokenKind::Ident(name) => {
                self.advance();
                name
            }
            _ => return Err(self.error("Expected identifier")),
        };

        let t = self.parse_type()?;
        Ok((key, t))
    }
}

#[cfg(test)]
mod tests {

    use super::*;
    use crate::types::types::{F64, I32, Prim, sarray, strct, union};

    /// Lex and parse a whole string as a type. Adapt the lexer call to your API.
    fn parse(src: &str) -> Result<Type<'_>> {
        let toks = crate::lexer::lex(src)?;
        let mut p = Parser::new(toks);
        let t = p.parse_type()?;
        if p.peek_kind() != TokenKind::Eof {
            Err(crate::error::Error::InvalidArgument)
        } else {
            Ok(t)
        }
    }

    fn green(src: &str, expected: Type<'_>) {
        match parse(src) {
            Ok(actual) => assert_eq!(actual, expected, "SHOULD PASS: {src}"),
            Err(e) => panic!("SHOULD PASS: {src}, got error {e:?}"),
        }
    }

    fn red(src: &str) {
        assert!(parse(src).is_err(), "SHOULD FAIL: {src}");
    }

    #[test]
    fn primitives() {
        use Prim::*;
        let cases = [
            ("i8", I8),
            ("i16", I16),
            ("i32", I32),
            ("i64", I64),
            ("u8", U8),
            ("u16", U16),
            ("u32", U32),
            ("u64", U64),
            ("f16", F16),
            ("f32", F32),
            ("f64", F64),
            ("f128", F128),
            ("cf32", CF32),
            ("cf64", CF64),
            ("cf128", CF128),
            ("cf256", CF256),
            ("ci16", CI16),
            ("ci32", CI32),
            ("ci64", CI64),
            ("ci128", CI128),
            ("cu16", CU16),
            ("cu32", CU32),
            ("cu64", CU64),
            ("cu128", CU128),
        ];
        for (src, p) in cases {
            green(src, Type::Prim(p));
        }
        red("i2");
    }

    #[test]
    fn sarrays() {
        green("[10]i32", sarray(&[10], I32));
        green("[5][10]f64", sarray(&[5, 10], F64));
        green("[2][3][4]u8", sarray(&[2, 3, 4], Type::Prim(Prim::U8)));
    }

    #[test]
    fn structs() {
        green(
            "struct { x i32, y f64 }",
            strct(vec![("x", I32), ("y", F64)]),
        );
        green(
            "struct { a struct { b i32 } }",
            strct(vec![("a", strct(vec![("b", I32)]))]),
        );
    }

    #[test]
    fn unions() {
        green(
            "union { x i32, y f64 }",
            union(vec![("x", I32), ("y", F64)]),
        );
    }

    #[test]
    fn complicated() {
        green(
            "[10]struct { x i32, y [5]f64 }",
            sarray(&[10], strct(vec![("x", I32), ("y", sarray(&[5], F64))])),
        );
    }

    #[test]
    fn syntax_errors() {
        red("");
        red("struct");
        red("struct {");
        red("struct { }");
        red("struct { x i32");
        red("struct { x i32, }");
        red("struct { x }");
        red("struct { i32 }");
        red("union { x i32 y f64 }");
        red("[10]");
        red("[]i32");
        red("[10 i32");
        red("i32 i32");
    }

    #[test]
    fn invalid_array_sizes() {
        red("[0]i32");
        red("[-1]i32");
        red("[2][0]i32");
    }

    #[test]
    fn duplicate_fields() {
        red("struct { x i32, x f64 }");
        red("union { a i32, a i32 }");
        green(
            "struct { x i32, y struct { x i32 } }",
            strct(vec![("x", I32), ("y", strct(vec![("x", I32)]))]),
        );
    }
}
