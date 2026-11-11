use std::num::NonZeroI64;

use super::Parser;
use crate::error::Result;
use crate::tokens::TokenKind;
use crate::types::user_stride::UserStride;

impl<'src> Parser<'src> {
    /// user_stride ::= '[' entry ']'
    pub fn parse_user_stride(&mut self) -> Result<UserStride> {
        // '['
        self.expect(TokenKind::LBracket)?;

        // '[1'
        let stride = if let Some(num) = self.maybe_parse_integer() {
            let start = Some(i64::from(num));

            // '[1:'
            if self.match_tokens(TokenKind::Colon) {
                // ...
                self.parse_us_stop(start)?

            // '[1'
            } else {
                UserStride::Index(i64::from(num))
            }

        // '[:'
        } else if self.match_tokens(TokenKind::Colon) {
            // ...
            self.parse_us_stop(None)?
        } else {
            return Err(self.error("expected number or ':'"));
        };

        // ...]
        self.expect(TokenKind::RBracket)?;
        Ok(stride)
    }

    /// multi_user_stride ::= '[' ( entry ( ',' entry )* )? ']'
    pub fn parse_multi_user_stride(&mut self) -> Result<Vec<UserStride>> {
        // [
        self.expect(TokenKind::LBracket)?;

        let mut strides = Vec::new();

        // []
        if self.match_tokens(TokenKind::RBracket) {
            return Ok(strides);
        }

        // [entry, entry, ...
        loop {
            strides.push(self.parse_us_entry()?);
            if !self.match_tokens(TokenKind::Comma) {
                break;
            }
        }

        // ...]
        self.expect(TokenKind::RBracket)?;
        Ok(strides)
    }

    /// entry ::= INTEGER ( ':' INTEGER? ( ':' INTEGER? )? )?
    ///         | ':' INTEGER? ( ':' INTEGER? )?
    fn parse_us_entry(&mut self) -> Result<UserStride> {
        if let Some(start) = self.maybe_parse_integer().map(i64::from) {
            if self.match_tokens(TokenKind::Colon) {
                self.parse_us_stop(Some(start)) // 'start:...'
            } else {
                Ok(UserStride::Index(start)) // 'start'
            }
        } else if self.match_tokens(TokenKind::Colon) {
            self.parse_us_stop(None) // ':...'
        } else {
            Err(self.error("expected number or ':'"))
        }
    }

    fn parse_us_step(&mut self) -> Result<Option<NonZeroI64>> {
        // '[...'
        if !self.match_tokens(TokenKind::Colon) {
            return Ok(None);
        }

        // '[...:'
        match self.maybe_parse_integer() {
            // '[...:1'
            Some(num) => NonZeroI64::new(num.into())
                .map(Some)
                .ok_or_else(|| self.error("step cannot be zero")),
            // '[...:'
            None => Ok(None),
        }
    }

    fn parse_us_stop(&mut self, start: Option<i64>) -> Result<UserStride> {
        let stop = self.maybe_parse_integer().map(i64::from);
        let step = self.parse_us_step()?;
        Ok(UserStride::Slice { start, stop, step })
    }
}

#[cfg(test)]
mod tests_user_stride {
    use std::num::NonZeroI64;

    use super::*;
    use crate::lexer::lex;

    /// Port of compile_user_stride
    fn compile(src: &str) -> Result<UserStride> {
        let tokens = lex(src)?;
        let mut parser = Parser::new(tokens); // adapt to Parser's constructor
        parser.parse_user_stride()
    }

    pub fn slice(start: Option<i64>, stop: Option<i64>, step: Option<i64>) -> UserStride {
        UserStride::Slice {
            start,
            stop,
            step: step.map(|s| NonZeroI64::new(s).unwrap()),
        }
    }

    fn green(src: &str, expected: UserStride) {
        assert_eq!(compile(src).unwrap(), expected, "SHOULD PASS: {src}");
    }

    fn red(src: &str) {
        assert!(compile(src).is_err(), "SHOULD FAIL: {src}");
    }

    #[test]
    fn compile_user_stride() {
        red("[]");
        red("[ ]");
        red(" [ ]");

        green("[5]", UserStride::Index(5));

        green("[:]", slice(None, None, None));
        green("[:6]", slice(None, Some(6), None));
        green("[5:]", slice(Some(5), None, None));
        green("[5:6]", slice(Some(5), Some(6), None));

        green("[::]", slice(None, None, None));
        green("[::7]", slice(None, None, Some(7)));
        green("[:6:]", slice(None, Some(6), None));
        green("[:6:7]", slice(None, Some(6), Some(7)));

        green("[5::]", slice(Some(5), None, None));
        green("[5::7]", slice(Some(5), None, Some(7)));
        green("[5:6:]", slice(Some(5), Some(6), None));
        green("[5:6:7]", slice(Some(5), Some(6), Some(7)));
    }

    pub fn compile_multi(src: &str) -> Result<Vec<UserStride>> {
        let tokens = lex(src)?;
        let mut parser = Parser::new(tokens);
        let strides = parser.parse_multi_user_stride()?;
        parser.check_end()?;
        Ok(strides)
    }

    fn green_multi(src: &str, expected: Vec<UserStride>) {
        match compile_multi(src) {
            Ok(actual) => assert_eq!(actual, expected, "SHOULD PASS: {src}"),
            Err(e) => panic!("SHOULD PASS: {src}, got error {e:?}"),
        }
    }

    fn red_multi(src: &str) {
        assert!(compile_multi(src).is_err(), "SHOULD FAIL: {src}");
    }

    #[test]
    fn compile_multi_user_stride() {
        use UserStride::Index;
        // Empty
        green_multi("[]", vec![]);
        green_multi("[ ]", vec![]);
        green_multi(" [ ]", vec![]);
        green_multi(" [] ", vec![]);

        // Bare indices
        green_multi("[0]", vec![Index(0)]);
        green_multi("[0, 0]", vec![Index(0), Index(0)]);

        // Slices, each followed by a bare index
        green_multi("[0:]", vec![slice(Some(0), None, None)]);
        green_multi("[0:, 0]", vec![slice(Some(0), None, None), Index(0)]);
        green_multi("[:, 0]", vec![slice(None, None, None), Index(0)]);
        green_multi("[:0, 0]", vec![slice(None, Some(0), None), Index(0)]);
        green_multi("[::, 0]", vec![slice(None, None, None), Index(0)]);
        green_multi("[::1, 0]", vec![slice(None, None, Some(1)), Index(0)]);
        green_multi("[:0:, 0]", vec![slice(None, Some(0), None), Index(0)]);
        green_multi("[:0:1, 0]", vec![slice(None, Some(0), Some(1)), Index(0)]);
        green_multi("[0::, 0]", vec![slice(Some(0), None, None), Index(0)]);
        green_multi("[0::1, 0]", vec![slice(Some(0), None, Some(1)), Index(0)]);
        green_multi(
            "[0:0:1, 0]",
            vec![slice(Some(0), Some(0), Some(1)), Index(0)],
        );

        // Errors
        red_multi("0, 1]"); // no leading '['
        red_multi("[0, 1"); // no trailing ']'
        red_multi(""); // empty input
        red_multi("[0,]"); // trailing comma
        red_multi("[,0]"); // leading comma
        red_multi("[0,,1]"); // double comma
        red_multi("[abc]"); // garbage token
        red_multi("42"); // no brackets
    }
}
