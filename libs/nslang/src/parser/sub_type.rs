//! Subtype grammar:
//!
//! ```text
//! subtype ::= IDENT ( multi_user_stride | '.' IDENT )*
//! ```

use super::Parser;
use crate::error::Result;
use crate::tokens::TokenKind;
use crate::types::sub_type::Subtype;
use crate::types::type_accessor::{self as ta, TypeAccessor};
use crate::types::user_stride::UserStride;

/// One link in the accessor chain, before it's nested
enum Step<'src> {
    Select(&'src str),
    Range(Vec<UserStride>),
}

impl<'src> Parser<'src> {
    /// subtype ::= IDENT ( multi_user_stride | '.' IDENT )*
    pub fn parse_subtype(&mut self) -> Result<Subtype<'src>> {
        // VNAME
        let TokenKind::Ident(vname) = self.peek_kind() else {
            return Err(self.error("Expected variable name"));
        };
        self.advance();

        let mut steps = Vec::new();

        loop {
            match self.peek_kind() {
                // [...]
                TokenKind::LBracket => {
                    let strides = self.parse_multi_user_stride()?;

                    // Consecutive brackets merge into one range, like
                    // tab_accept_stride: a[0][1] is one Range of two strides
                    if let Some(Step::Range(prev)) = steps.last_mut() {
                        prev.extend(strides);
                    } else {
                        steps.push(Step::Range(strides));
                    }
                }

                // .IDENT
                TokenKind::Dot => {
                    self.advance();
                    let TokenKind::Ident(key) = self.peek_kind() else {
                        return Err(self.error("Expected identifier"));
                    };
                    self.advance();
                    steps.push(Step::Select(key));
                }

                // Done
                _ => break,
            }
        }

        // Nest from the inside out: the last step wraps Take
        let accessor = steps
            .into_iter()
            .rev()
            .fold(TypeAccessor::Take, |sub, step| match step {
                Step::Select(key) => ta::select(key, sub),
                Step::Range(strides) => ta::range(strides, sub),
            });

        Ok(Subtype::new(vname, accessor))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Port of compile_subtype
    fn parse(src: &str) -> Result<Subtype<'_>> {
        let toks = crate::lexer::lex(src)?;
        let mut p = Parser::new(toks);
        let st = p.parse_subtype()?;
        p.check_end()?;
        Ok(st)
    }

    fn green(src: &str, expected: Subtype<'_>) {
        match parse(src) {
            Ok(actual) => assert_eq!(actual, expected, "SHOULD PASS: {src}"),
            Err(e) => panic!("SHOULD PASS: {src}, got error {e:?}"),
        }
    }

    fn red(src: &str) {
        assert!(parse(src).is_err(), "SHOULD FAIL: {src}");
    }

    #[test]
    fn compile_subtype() {
        green("myvar", Subtype::new("myvar", ta::take()));

        green(
            "myvar[9]",
            Subtype::new("myvar", ta::range(vec![UserStride::Index(9)], ta::take())),
        );

        green(
            "myvar.field",
            Subtype::new("myvar", ta::select("field", ta::take())),
        );

        green(
            "myvar.a[0]",
            Subtype::new(
                "myvar",
                ta::select("a", ta::range(vec![UserStride::Index(0)], ta::take())),
            ),
        );

        red("");
        red("42");
        red("[0]");
        red("struct a x }");
        red("foo.");
        red(".bar");
    }
}
