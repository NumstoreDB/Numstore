mod query;
mod sub_type;
mod type_ref;
mod types;
mod user_stride;

use crate::{
    error::{Error, ParserError, Result},
    tokens::{Token, TokenKind},
};

pub struct Parser<'src> {
    toks: Vec<Token<'src>>,
    pos: usize,
}

impl<'src> Parser<'src> {
    pub fn new(toks: Vec<Token<'src>>) -> Self {
        Parser { toks, pos: 0 }
    }

    fn peek(&self) -> &Token<'src> {
        assert!(self.pos < self.toks.len());
        &self.toks[self.pos]
    }

    fn peek_kind(&self) -> TokenKind<'src> {
        assert!(self.pos < self.toks.len());
        self.peek().kind
    }

    fn peek_next(&self) -> &Token<'src> {
        assert!(self.pos + 1 < self.toks.len());
        let target_pos = self.pos + 1;
        &self.toks[target_pos]
    }

    fn match_tokens(&mut self, kind: TokenKind<'src>) -> bool {
        if self.peek().kind == kind {
            self.advance();
            true
        } else {
            false
        }
    }

    fn advance(&mut self) -> &Token<'src> {
        assert!(self.pos < self.toks.len());
        let tok = &self.toks[self.pos];
        self.pos += 1;
        tok
    }

    fn maybe_parse_integer(&mut self) -> Option<i32> {
        match self.peek().kind {
            TokenKind::Int(val) => {
                self.advance();
                Some(val)
            }
            TokenKind::Minus => {
                // Valid because next is at least eof
                match self.peek_next().kind {
                    TokenKind::Int(val) => {
                        self.advance(); // '-'
                        self.advance(); // integer
                        Some(-val)
                    }
                    _ => None,
                }
            }
            _ => None,
        }
    }

    fn expect(&mut self, kind: TokenKind<'src>) -> Result<()> {
        let next_kind = self.peek().kind;

        if next_kind != kind {
            return Err(self.error(format!(
                "Expected token type {kind:?} but got {next_kind:?}"
            )));
        }

        self.pos += 1;
        Ok(())
    }

    fn at_end(&self) -> bool {
        self.peek_kind() == TokenKind::Eof
    }

    pub(crate) fn check_end(&self) -> Result<()> {
        if !self.at_end() {
            Err(self.error(format!("Unexpected tokens at position {:?}", self.pos)))
        } else {
            Ok(())
        }
    }

    fn error(&self, message: impl Into<String>) -> Error {
        Error::Parse(ParserError::new(message, &self.toks, self.pos))
    }
}
