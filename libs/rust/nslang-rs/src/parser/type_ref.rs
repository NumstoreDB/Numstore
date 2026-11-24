//! Type ref grammar:
//!
//! ```text
//! type_ref        ::= struct_type_ref | take_type_ref
//! struct_type_ref ::= 'struct' '{' field_ref ( ',' field_ref )* '}'
//! field_ref       ::= IDENT type_ref
//! take_type_ref   ::= subtype
//! ```

use super::Parser;
use crate::error::Result;
use crate::tokens::{Keyword, TokenKind};
use crate::types::type_ref::{StructRef, TypeRef};

impl<'src> Parser<'src> {
    /// type_ref ::= struct_type_ref | take_type_ref
    pub fn parse_type_ref(&mut self) -> Result<TypeRef<'src>> {
        match self.peek_kind() {
            // struct { ...
            TokenKind::Keyword(Keyword::Struct) => self.parse_struct_type_ref(),

            // myvar.a[0]
            TokenKind::Ident(_) => Ok(TypeRef::Take(self.parse_subtype()?)),

            other => Err(self.error(format!(
                "Expected type ref (struct or identifier), got {other}"
            ))),
        }
    }

    /// struct_type_ref ::= 'struct' '{' field_ref ( ',' field_ref )* '}'
    fn parse_struct_type_ref(&mut self) -> Result<TypeRef<'src>> {
        // struct {
        self.expect(TokenKind::Keyword(Keyword::Struct))?;
        self.expect(TokenKind::LBrace)?;

        let mut keys = Vec::new();
        let mut types = Vec::new();

        loop {
            // foo TYPE_REF
            let (key, t) = self.parse_field_ref()?;

            if keys.contains(&key) {
                return Err(self.error(format!("Duplicate field name '{key}'")));
            }

            keys.push(key);
            types.push(t);

            // foo TYPE_REF,
            if !self.match_tokens(TokenKind::Comma) {
                break;
            }
        }

        // }
        self.expect(TokenKind::RBrace)?;

        Ok(TypeRef::Struct(StructRef {
            keys: keys.into_boxed_slice(),
            types: types.into_boxed_slice(),
        }))
    }

    /// field_ref ::= IDENT type_ref
    fn parse_field_ref(&mut self) -> Result<(&'src str, TypeRef<'src>)> {
        let TokenKind::Ident(key) = self.peek_kind() else {
            return Err(self.error("Expected identifier"));
        };
        self.advance();

        Ok((key, self.parse_type_ref()?))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::types::type_accessor as ta;
    use crate::types::type_ref as tr;
    use crate::types::user_stride::UserStride;

    /// Port of compile_type_ref
    fn parse(src: &str) -> Result<TypeRef<'_>> {
        let toks = crate::lexer::lex(src)?;
        let mut p = Parser::new(toks);
        let t = p.parse_type_ref()?;
        p.check_end()?;
        Ok(t)
    }

    fn green(src: &str, expected: TypeRef<'_>) {
        match parse(src) {
            Ok(actual) => assert_eq!(actual, expected, "SHOULD PASS: {src}"),
            Err(e) => panic!("SHOULD PASS: {src}, got error {e:?}"),
        }
    }

    fn red(src: &str) {
        assert!(parse(src).is_err(), "SHOULD FAIL: {src}");
    }

    #[test]
    fn compile_type_ref() {
        green("myvar", tr::take("myvar", ta::take()));

        green(
            "myvar[9]",
            tr::take("myvar", ta::range(vec![UserStride::Index(9)], ta::take())),
        );

        green(
            "myvar.field",
            tr::take("myvar", ta::select("field", ta::take())),
        );

        green(
            "myvar.a[0]",
            tr::take(
                "myvar",
                ta::select("a", ta::range(vec![UserStride::Index(0)], ta::take())),
            ),
        );

        green(
            "struct { a myvar }",
            tr::strct_ref(vec![("a", tr::take("myvar", ta::take()))]),
        );

        green(
            "struct { a x, b y }",
            tr::strct_ref(vec![
                ("a", tr::take("x", ta::take())),
                ("b", tr::take("y", ta::take())),
            ]),
        );

        green(
            "struct { a x, b y, c z }",
            tr::strct_ref(vec![
                ("a", tr::take("x", ta::take())),
                ("b", tr::take("y", ta::take())),
                ("c", tr::take("z", ta::take())),
            ]),
        );

        green(
            "struct { a struct { b x } }",
            tr::strct_ref(vec![(
                "a",
                tr::strct_ref(vec![("b", tr::take("x", ta::take()))]),
            )]),
        );

        green(
            "struct { a myvar[0] }",
            tr::strct_ref(vec![(
                "a",
                tr::take("myvar", ta::range(vec![UserStride::Index(0)], ta::take())),
            )]),
        );

        red("");
        red("42");
        red("[0]");
        red("struct a x }");
        red("struct {}");
        red("struct { a }");
        red("struct { a x");
        red("struct { a x, }");
        red("struct { a x,, b y }");
        red("struct { a x, b }");
        red("struct { a x, struct y }");
    }
}
