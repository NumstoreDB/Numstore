use super::Parser;
use crate::error::Result;
use crate::tokens::{Keyword, TokenKind};
use crate::types::query::{Query, QueryKind};
use crate::types::user_stride::UserStride;

impl<'src> Parser<'src> {
    pub fn parse_query(&mut self) -> Result<Query<'src>> {
        let TokenKind::Keyword(kw) = self.peek_kind() else {
            return Err(self.error("Expected a valid operation"));
        };

        match kw {
            Keyword::Help => self.parse_query_help(),

            Keyword::Exit => {
                self.advance();
                Ok(Query::Exit)
            }

            Keyword::Get => {
                self.advance();
                let if_exists = self.parse_if_exists()?;
                let name = self.expect_ident()?;
                Ok(Query::Get { name, if_exists })
            }

            Keyword::Delete => {
                self.advance();
                let if_exists = self.parse_if_exists()?;
                let name = self.expect_ident()?;
                Ok(Query::Delete { name, if_exists })
            }

            Keyword::Create => {
                self.advance();
                let name = self.expect_ident()?;
                let ty = self.parse_type()?;
                Ok(Query::Create { name, ty })
            }

            Keyword::Insert => {
                self.advance();
                let name = self.expect_ident()?;
                let ofst = i64::from(self.expect_integer()?);
                let len = self.expect_integer()?;
                let len = u64::try_from(len)
                    .map_err(|_| self.error("Insert length cannot be negative"))?;
                Ok(Query::Insert { name, ofst, len })
            }

            Keyword::Read => {
                self.advance();
                let (name, ustr) = self.parse_name_and_ustr()?;
                Ok(Query::Read { name, ustr })
            }

            Keyword::Remove => {
                self.advance();
                let (name, ustr) = self.parse_name_and_ustr()?;
                Ok(Query::Remove { name, ustr })
            }

            Keyword::Write => {
                self.advance();
                let (name, ustr) = self.parse_name_and_ustr()?;
                Ok(Query::Write { name, ustr })
            }

            _ => Err(self.error("Expected a valid operation")),
        }
    }

    /// 'help' ( 'read' | 'create' | 'delete' | 'get' | 'exit' | 'help' )?
    fn parse_query_help(&mut self) -> Result<Query<'src>> {
        self.expect(TokenKind::Keyword(Keyword::Help))?;

        let command = match self.peek_kind() {
            TokenKind::Keyword(Keyword::Read) => Some(QueryKind::Read),
            TokenKind::Keyword(Keyword::Create) => Some(QueryKind::Create),
            TokenKind::Keyword(Keyword::Delete) => Some(QueryKind::Delete),
            TokenKind::Keyword(Keyword::Get) => Some(QueryKind::Get),
            TokenKind::Keyword(Keyword::Exit) => Some(QueryKind::Exit),
            TokenKind::Keyword(Keyword::Help) => Some(QueryKind::Help),
            _ => None,
        };

        if command.is_some() {
            self.advance();
        }

        Ok(Query::Help { command })
    }

    /// ( 'if' 'exists' )?
    fn parse_if_exists(&mut self) -> Result<bool> {
        if !self.match_tokens(TokenKind::Keyword(Keyword::If)) {
            return Ok(false);
        }
        self.expect(TokenKind::Keyword(Keyword::Exists))?;
        Ok(true)
    }

    /// IDENT user_stride?  (no stride means the whole array, `[:]`)
    fn parse_name_and_ustr(&mut self) -> Result<(&'src str, UserStride)> {
        let name = self.expect_ident()?;

        let ustr = if self.peek_kind() == TokenKind::LBracket {
            self.parse_user_stride()?
        } else {
            UserStride::Slice {
                start: None,
                stop: None,
                step: None,
            }
        };

        Ok((name, ustr))
    }

    fn expect_ident(&mut self) -> Result<&'src str> {
        let TokenKind::Ident(name) = self.peek_kind() else {
            return Err(self.error("Expected identifier"));
        };
        self.advance();
        Ok(name)
    }

    fn expect_integer(&mut self) -> Result<i32> {
        let Some(v) = self.maybe_parse_integer() else {
            return Err(self.error("Expected integer"));
        };
        Ok(v)
    }
}

#[cfg(test)]
mod tests {
    use std::num::NonZeroI64;

    use super::*;
    use crate::types::types::{F32, I32, sarray, strct, union};

    /// Port of compile_query
    fn parse(src: &str) -> Result<Query<'_>> {
        let toks = crate::lexer::lex(src)?;
        let mut p = Parser::new(toks);
        let q = p.parse_query()?;
        p.check_end()?;
        Ok(q)
    }

    fn green(src: &str, expected: Query<'_>) {
        match parse(src) {
            Ok(actual) => assert_eq!(actual, expected, "SHOULD PASS: {src}"),
            Err(e) => panic!("SHOULD PASS: {src}, got error {e:?}"),
        }
    }

    fn red(src: &str) {
        assert!(parse(src).is_err(), "SHOULD FAIL: {src}");
    }

    /// Port of ustride(): the whole array
    fn all() -> UserStride {
        UserStride::Slice {
            start: None,
            stop: None,
            step: None,
        }
    }

    /// Port of ustride012
    fn ustride012(start: i64, stop: i64, step: i64) -> UserStride {
        UserStride::Slice {
            start: Some(start),
            stop: Some(stop),
            step: NonZeroI64::new(step),
        }
    }

    #[test]
    fn compile_query_read() {
        red("read");
        red("read 10");
        green(
            "read foo",
            Query::Read {
                name: "foo",
                ustr: all(),
            },
        );
        green(
            "read foo[0:10:20]",
            Query::Read {
                name: "foo",
                ustr: ustride012(0, 10, 20),
            },
        );
    }

    #[test]
    fn compile_query_remove() {
        red("remove");
        red("remove 10");
        green(
            "remove foo",
            Query::Remove {
                name: "foo",
                ustr: all(),
            },
        );
        green(
            "remove foo[0:10:20]",
            Query::Remove {
                name: "foo",
                ustr: ustride012(0, 10, 20),
            },
        );
    }

    #[test]
    fn compile_query_write() {
        red("write");
        red("write 10");
        green(
            "write foo",
            Query::Write {
                name: "foo",
                ustr: all(),
            },
        );
        green(
            "write foo[0:10:20]",
            Query::Write {
                name: "foo",
                ustr: ustride012(0, 10, 20),
            },
        );
    }

    #[test]
    fn compile_query_insert() {
        red("insert");
        red("insert foo");
        red("insert foo 0");
        green(
            "insert foo 0 1",
            Query::Insert {
                name: "foo",
                ofst: 0,
                len: 1,
            },
        );
    }

    #[test]
    fn compile_query_create() {
        red("create");
        red("create 1");
        red("create foo");
        red("create foo 1");
        green(
            "create foo struct { a i32, b union { c i32, d [10][20]f32 } }",
            Query::Create {
                name: "foo",
                ty: strct(vec![
                    ("a", I32),
                    ("b", union(vec![("c", I32), ("d", sarray(&[10, 20], F32))])),
                ]),
            },
        );
    }

    #[test]
    fn compile_query_delete() {
        red("delete");
        red("delete 1");
        green(
            "delete foo",
            Query::Delete {
                name: "foo",
                if_exists: false,
            },
        );
    }

    #[test]
    fn compile_query_get() {
        red("get");
        red("get 1");
        red("get if");
        red("get if foo");
        green(
            "get foo",
            Query::Get {
                name: "foo",
                if_exists: false,
            },
        );
        green(
            "get if exists foo",
            Query::Get {
                name: "foo",
                if_exists: true,
            },
        );
    }

    #[test]
    fn compile_query_exit() {
        green("exit", Query::Exit);
    }

    #[test]
    fn compile_query_help() {
        green("help", Query::Help { command: None });
        green(
            "help read",
            Query::Help {
                command: Some(QueryKind::Read),
            },
        );
        green(
            "help create",
            Query::Help {
                command: Some(QueryKind::Create),
            },
        );
        green(
            "help delete",
            Query::Help {
                command: Some(QueryKind::Delete),
            },
        );
        green(
            "help get",
            Query::Help {
                command: Some(QueryKind::Get),
            },
        );
        green(
            "help exit",
            Query::Help {
                command: Some(QueryKind::Exit),
            },
        );
        green(
            "help help",
            Query::Help {
                command: Some(QueryKind::Help),
            },
        );
    }
}
