use std::fmt;

use crate::word_enum;

use crate::types::types::Prim;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash, Default)]
pub struct Span {
    pub start: usize,
    pub len: usize,
}

impl Span {
    pub fn new(start: usize, len: usize) -> Self {
        Span { start, len }
    }

    /// The text this span covers.
    pub fn slice(self, src: &str) -> &str {
        &src[self.start..self.start + self.len]
    }
}

impl fmt::Display for Span {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}..{}", self.start, self.start + self.len)
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct Token<'src> {
    pub kind: TokenKind<'src>,
    pub span: Span,
}

impl fmt::Display for Token<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{} at position {}", self.kind, self.span.start)
    }
}

impl<'src> Token<'src> {
    pub fn new(kind: TokenKind<'src>, span: Span) -> Self {
        Token { kind, span }
    }
}

word_enum! {
    Keyword {
        Create => "create",
        Delete => "delete",
        Get    => "get",
        Insert => "insert",
        Read   => "read",
        Write  => "write",
        Remove => "remove",
        Struct => "struct",
        Union  => "union",
        If     => "if",
        Exists => "exists",
        Exit   => "exit",
        Help   => "help",
        True   => "true",
        False  => "false",
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum TokenKind<'src> {
    // Arithmetic
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Tilde,

    // Comparison / logic
    Bang,
    BangEqual,
    EqualEqual,
    Greater,
    GreaterEqual,
    Less,
    LessEqual,
    Caret,
    Pipe,
    PipePipe,
    Amp,
    AmpAmp,

    // Punctuation
    Semicolon,
    Colon,
    Comma,
    Dot,
    LBracket,
    RBracket,
    LBrace,
    RBrace,
    LParen,
    RParen,

    // Literals and names
    Ident(&'src str),
    Str(&'src str),
    Int(i32),
    Float(f32),

    Keyword(Keyword),
    Prim(Prim),

    Eof,
}

impl TokenKind<'_> {
    /// Classify a word: keyword, boolean, primitive type, or identifier.
    pub fn from_word(word: &str) -> TokenKind<'_> {
        match word {
            _ => {
                if let Ok(kw) = word.parse() {
                    TokenKind::Keyword(kw)
                } else if let Ok(p) = word.parse() {
                    TokenKind::Prim(p)
                } else {
                    TokenKind::Ident(word)
                }
            }
        }
    }
}

impl fmt::Display for TokenKind<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        use TokenKind::*;
        let sym = match self {
            Plus => "+",
            Minus => "-",
            Star => "*",
            Slash => "/",
            Percent => "%",
            Tilde => "~",
            Bang => "!",
            BangEqual => "!=",
            EqualEqual => "==",
            Greater => ">",
            GreaterEqual => ">=",
            Less => "<",
            LessEqual => "<=",
            Caret => "^",
            Pipe => "|",
            PipePipe => "||",
            Amp => "&",
            AmpAmp => "&&",
            Semicolon => ";",
            Colon => ":",
            Comma => ",",
            Dot => ".",
            LBracket => "[",
            RBracket => "]",
            LBrace => "{",
            RBrace => "}",
            LParen => "(",
            RParen => ")",

            Ident(s) => return write!(f, "identifier '{s}'"),
            Str(s) => return write!(f, "string \"{s}\""),
            Int(n) => return write!(f, "integer {n}"),
            Float(x) => return write!(f, "float {x}"),
            Keyword(k) => return write!(f, "keyword '{k}'"),
            Prim(p) => return write!(f, "type '{p}'"),
            Eof => return f.write_str("end of input"),
        };
        write!(f, "'{sym}'")
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn tt_tostr() {
        use TokenKind::*;
        let mut every: Vec<TokenKind> = vec![
            Plus,
            Minus,
            Star,
            Slash,
            Percent,
            Tilde,
            Bang,
            BangEqual,
            EqualEqual,
            Greater,
            GreaterEqual,
            Less,
            LessEqual,
            Caret,
            Pipe,
            PipePipe,
            Amp,
            AmpAmp,
            Semicolon,
            Colon,
            Comma,
            Dot,
            LBracket,
            RBracket,
            LBrace,
            RBrace,
            LParen,
            RParen,
            Ident("foo"),
            Str("bar"),
            Int(42),
            Float(1.5),
            Eof,
        ];
        every.extend(super::Keyword::ALL.iter().map(|&k| TokenKind::Keyword(k)));
        every.extend(super::Prim::ALL.iter().map(|&p| TokenKind::Prim(p)));

        for kind in every {
            let s = kind.to_string();
            assert!(!s.is_empty());
            println!("{s}");
        }
    }
}
