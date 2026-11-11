use crate::error::Result;
use crate::lexer::lex;
use crate::parser::Parser;
use crate::tokens::Keyword;
use crate::types::query::Query;
use crate::types::sub_type::Subtype;
use crate::types::type_ref::TypeRef;
use crate::types::types::{Prim, Type};
use crate::types::user_stride::UserStride;

fn compile<'src, T>(
    src: &'src str,
    parse: impl FnOnce(&mut Parser<'src>) -> Result<T>,
) -> Result<T> {
    let mut parser = Parser::new(lex(src)?);
    let out = parse(&mut parser)?;
    parser.check_end()?;
    Ok(out)
}

pub fn compile_type(src: &str) -> Result<Type<'_>> {
    compile(src, Parser::parse_type)
}

pub fn compile_subtype(src: &str) -> Result<Subtype<'_>> {
    compile(src, Parser::parse_subtype)
}

pub fn compile_type_ref(src: &str) -> Result<TypeRef<'_>> {
    compile(src, Parser::parse_type_ref)
}

pub fn compile_query(src: &str) -> Result<Query<'_>> {
    Parser::new(lex(src)?).parse_query()
}

pub fn compile_user_stride(src: &str) -> Result<UserStride> {
    compile(src, Parser::parse_user_stride)
}

pub fn compile_multi_user_stride(src: &str) -> Result<Vec<UserStride>> {
    compile(src, Parser::parse_multi_user_stride)
}

pub fn is_keyword(word: &str) -> bool {
    word.parse::<Keyword>().is_ok() || word.parse::<Prim>().is_ok()
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Port of TEST (lex_is_keyword)
    #[test]
    fn keywords_are_reserved() {
        for word in [
            "create", "delete", "if", "exists", "struct", "union", "true", "false",
        ] {
            assert!(is_keyword(word), "{word}");
        }

        // A word is scanned as a primitive type before it is checked against
        // the keywords, so primitive names are reserved too
        for word in ["u8", "i8", "f64", "cf128"] {
            assert!(is_keyword(word), "{word}");
        }

        // Ordinary identifiers are not, and only an exact match counts
        for word in ["foo", "_x", "iff", "u8x", "creates", ""] {
            assert!(!is_keyword(word), "{word}");
        }
    }

    #[test]
    fn compile_requires_the_whole_text() {
        assert!(compile_type("u32").is_ok());
        assert!(compile_type("u32 u32").is_err());
        assert!(compile_user_stride("[1:2").is_err());

        // A query is the exception: what follows the statement is dropped
        assert_eq!(
            compile_query("get foo bar").unwrap(),
            compile_query("get foo").unwrap()
        );
        assert!(compile_query("create foo u32;").is_ok());
        assert!(compile_query("read foo[0:] blimit 8").is_ok());
        assert!(compile_user_stride("[1:2] 3").is_err());
        assert!(compile_multi_user_stride("[1, 2]").is_ok());
        assert!(compile_subtype("a.b[0]").is_ok());
        assert!(compile_type_ref("struct { a b }").is_ok());
    }
}
