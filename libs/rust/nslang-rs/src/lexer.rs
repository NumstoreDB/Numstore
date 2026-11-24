use crate::{
    error::{Error, LexerError, Result},
    tokens::{Span, Token, TokenKind},
};

/// Lex an entire string into a vector of tokens
pub fn lex(src: &str) -> Result<Vec<Token<'_>>> {
    let mut tokens: Vec<Token> = Vec::new();

    // Lexers are temporary
    let mut lexer = Lexer::new(src);

    while !lexer.is_at_end() {
        lexer.start = lexer.pos;
        lexer.scan_next(&mut tokens)?
    }

    lexer.start = lexer.pos;
    lexer.append_token(&mut tokens, TokenKind::Eof)?;
    Ok(tokens)
}

struct Lexer<'src> {
    src: &'src str,
    start: usize,
    pos: usize,
}

impl<'src> Lexer<'src> {
    fn new(src: &'src str) -> Self {
        Lexer {
            src,
            start: 0,
            pos: 0,
        }
    }

    fn is_at_end(&self) -> bool {
        self.pos >= self.src.len()
    }

    /// Peek 1 byte forward
    fn peek(&self) -> Option<u8> {
        if self.is_at_end() {
            None
        } else {
            Some(self.src.as_bytes()[self.pos])
        }
    }

    /// Peek 2 bytes forward
    fn peek_next(&self) -> Option<u8> {
        if self.pos + 1 >= self.src.len() {
            None
        } else {
            Some(self.src.as_bytes()[self.pos + 1])
        }
    }

    /// Move forward 1
    fn advance(&mut self) -> u8 {
        assert!(!self.is_at_end());

        let ret = self.src.as_bytes()[self.pos];
        self.pos += 1;
        ret
    }

    /// If next token == expected, move forward else don't
    fn match_token(&mut self, expected: u8) -> bool {
        assert!(!self.is_at_end());

        if self.src.as_bytes()[self.pos] != expected {
            false
        } else {
            self.pos += 1;
            true
        }
    }

    /// Append next token to the resulting tokens
    fn scan_next(&mut self, tokens: &mut Vec<Token<'src>>) -> Result<()> {
        assert!(self.pos == self.start);
        assert!(!self.is_at_end());

        use TokenKind::*;

        let c = self.advance();

        match c {
            // Simple tokens
            b' ' | b'\r' | b'\t' | b'\n' => Ok(()),
            b'+' => self.append_token(tokens, Plus),
            b'-' => self.append_token(tokens, Minus),
            b'*' => self.append_token(tokens, Star),
            b'/' => self.append_token(tokens, Slash),
            b'%' => self.append_token(tokens, Percent),
            b'~' => self.append_token(tokens, Tilde),
            b'^' => self.append_token(tokens, Caret),
            b';' => self.append_token(tokens, Semicolon),
            b':' => self.append_token(tokens, Colon),
            b',' => self.append_token(tokens, Comma),
            b'.' => self.append_token(tokens, Dot),
            b'[' => self.append_token(tokens, LBracket),
            b']' => self.append_token(tokens, RBracket),
            b'{' => self.append_token(tokens, LBrace),
            b'}' => self.append_token(tokens, RBrace),
            b'(' => self.append_token(tokens, LParen),
            b')' => self.append_token(tokens, RParen),

            // Tokens that have optional trailing chars
            b'!' => self.append_match_trail(tokens, b'=', BangEqual, Bang),
            b'>' => self.append_match_trail(tokens, b'=', GreaterEqual, Greater),
            b'<' => self.append_match_trail(tokens, b'=', LessEqual, Less),
            b'|' => self.append_match_trail(tokens, b'|', PipePipe, Pipe),
            b'&' => self.append_match_trail(tokens, b'&', AmpAmp, Amp),

            // Equals
            b'=' => {
                if self.is_at_end() || !self.match_token(b'=') {
                    Err(self.error("Invalid token: ="))
                } else {
                    self.append_token(tokens, EqualEqual)
                }
            }

            // Strings
            b'"' => self.append_string(tokens),

            // Numbers
            b'0'..=b'9' => self.append_number(tokens),

            // Identifiers
            c if c.is_ascii_alphabetic() || c == b'_' => self.append_ident(tokens),

            // Error
            _ => {
                // Report the whole character, not just its first byte.
                let ch = self.src[self.start..].chars().next().unwrap();
                self.pos = self.start + ch.len_utf8();
                return Err(self.error("Invalid character"));
            }
        }
    }

    /// Append a token to result
    fn append_token(&mut self, tokens: &mut Vec<Token<'src>>, kind: TokenKind<'src>) -> Result<()> {
        tokens.try_reserve(1)?;

        tokens.push(Token {
            kind: kind,
            span: Span {
                start: self.start,
                len: self.pos - self.start,
            },
        });

        Ok(())
    }

    /// Append [without], or if next is trailing, append [with]
    fn append_match_trail(
        &mut self,
        tokens: &mut Vec<Token<'src>>,
        trailing: u8,
        with: TokenKind<'src>,
        without: TokenKind<'src>,
    ) -> Result<()> {
        if !self.is_at_end() && self.match_token(trailing) {
            self.append_token(tokens, with)
        } else {
            self.append_token(tokens, without)
        }
    }

    /// Append a string token
    fn append_string(&mut self, tokens: &mut Vec<Token<'src>>) -> Result<()> {
        while !self.is_at_end() && self.peek() != Some(b'"') {
            self.advance();
        }

        if self.is_at_end() {
            return Err(self.error("Unterminated string"));
        }

        self.advance();

        self.append_token(
            tokens,
            TokenKind::Str(&self.src[self.start + 1..self.pos - 1]),
        )
    }

    /// Append a number
    fn append_number(&mut self, tokens: &mut Vec<Token<'src>>) -> Result<()> {
        while !self.is_at_end() && self.peek().is_some_and(|c| c.is_ascii_digit()) {
            self.advance();
        }

        let next_is_dot = self.peek() == Some(b'.');
        let has_number = self.peek_next().is_some_and(|c| c.is_ascii_digit());
        let is_float = next_is_dot && has_number;

        if is_float {
            // Skip the dot
            self.advance();

            while !self.is_at_end() && self.peek().is_some_and(|c| c.is_ascii_digit()) {
                self.advance();
            }
        }

        let text = &self.src[self.start..self.pos];

        let token = if is_float {
            text.parse()
                .map(TokenKind::Float)
                .map_err(|_| self.error("Failed to parse float"))
        } else {
            text.parse()
                .map(TokenKind::Int)
                .map_err(|_| self.error("Failed to parse int"))
        }?;

        self.append_token(tokens, token)
    }

    /// Append an identifier
    fn append_ident(&mut self, tokens: &mut Vec<Token<'src>>) -> Result<()> {
        while !self.is_at_end()
            && self
                .peek()
                .is_some_and(|c| c.is_ascii_alphanumeric() || c == b'_')
        {
            self.advance();
        }
        self.append_token(
            tokens,
            TokenKind::from_word(&self.src[self.start..self.pos]),
        )
    }

    fn error(&mut self, message: &str) -> Error {
        Error::Lex(LexerError::new(message, self.src, self.start, self.pos).into())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::tokens::Keyword::*;
    use crate::tokens::TokenKind::*;
    use crate::types::types::Prim::*;

    fn assert_lex(src: &str, expected: &[TokenKind]) {
        let toks = lex(src).unwrap();
        let kinds: Vec<_> = toks.iter().map(|t| &t.kind).collect();

        println!("{:?}", expected);
        println!("{:?}", kinds);
        assert_eq!(kinds.len(), expected.len() + 1);
        assert_eq!(kinds.last(), Some(&&Eof));
        for (i, (got, want)) in kinds.iter().zip(expected).enumerate() {
            assert_eq!(*got, want, "token {i} of {src:?}");
        }
    }

    fn assert_lex_err(src: &str) {
        assert!(lex(src).is_err());
    }

    #[test]
    fn lexer_simple() {
        let toks = lex("create a u32").unwrap();

        let create = Token::new(TokenKind::Keyword(Create), Span::new(0, 6));
        let a = Token::new(TokenKind::Ident("a"), Span::new(7, 1));
        let _u32 = Token::new(TokenKind::Prim(U32), Span::new(9, 3));
        let eof = Token::new(TokenKind::Eof, Span::new(12, 0));
        let expected = vec![create, a, _u32, eof];

        assert_eq!(toks, expected);
    }

    #[test]
    fn lexer_two_char_tokens() {
        assert_lex(
            "! ! != != == ! < <= > >= || && , ! , !=",
            &[
                Bang,
                Bang,
                BangEqual,
                BangEqual,
                EqualEqual,
                Bang,
                Less,
                LessEqual,
                Greater,
                GreaterEqual,
                PipePipe,
                AmpAmp,
                Comma,
                Bang,
                Comma,
                BangEqual,
            ],
        );
    }

    #[test]
    fn lexer_single_char_operators() {
        assert_lex(
            "+ - / * ~ ^ % | & ; : [ ] { } ( ) ,",
            &[
                Plus, Minus, Slash, Star, Tilde, Caret, Percent, Pipe, Amp, Semicolon, Colon,
                LBracket, RBracket, LBrace, RBrace, LParen, RParen, Comma,
            ],
        );
    }

    #[test]
    fn lexer_strings() {
        assert_lex(
            "\"hello\" \"world\" \"foo bar\"",
            &[Str("hello"), Str("world"), Str("foo bar")],
        );
    }

    #[test]
    fn lexer_identifiers() {
        assert_lex(
            "foo bar baz_qux hello123",
            &[
                Ident("foo"),
                Ident("bar"),
                Ident("baz_qux"),
                Ident("hello123"),
            ],
        );
    }

    #[test]
    fn lexer_numbers() {
        assert_lex(
            "0 123 456 1.0 3.14 0.5",
            &[
                Int(0),
                Int(123),
                Int(456),
                Float(1.0),
                Float(3.14),
                Float(0.5),
            ],
        );
    }

    #[test]
    fn lexer_keywords() {
        assert_lex(
            "create delete get exit help insert if exists struct union true false",
            &[
                Keyword(Create),
                Keyword(Delete),
                Keyword(Get),
                Keyword(Exit),
                Keyword(Help),
                Keyword(Insert),
                Keyword(If),
                Keyword(Exists),
                Keyword(Struct),
                Keyword(Union),
                Keyword(True),
                Keyword(False),
            ],
        );
    }

    #[test]
    fn lexer_primitives() {
        assert_lex(
            "u8 u16 u32 u64 i8 i16 i32 i64 f32 f64",
            &[
                Prim(U8),
                Prim(U16),
                Prim(U32),
                Prim(U64),
                Prim(I8),
                Prim(I16),
                Prim(I32),
                Prim(I64),
                Prim(F32),
                Prim(F64),
            ],
        );
    }

    #[test]
    fn lexer_whitespace_handling() {
        let expected = [Ident("a"), Plus, Ident("b")];
        assert_lex("a+b", &expected);
        assert_lex("a + b", &expected);
        assert_lex("  a  +  b  ", &expected);
    }

    #[test]
    fn lexer_complex_expression() {
        assert_lex(
            "create foo { x: u32, y: f32 };",
            &[
                Keyword(Create),
                Ident("foo"),
                LBrace,
                Ident("x"),
                Colon,
                Prim(U32),
                Comma,
                Ident("y"),
                Colon,
                Prim(F32),
                RBrace,
                Semicolon,
            ],
        );
    }

    #[test]
    fn lexer_keyword_prefix() {
        assert_lex(
            "create crate createx truex falsey",
            &[
                Keyword(Create),
                Ident("crate"),
                Ident("createx"),
                Ident("truex"),
                Ident("falsey"),
            ],
        );
    }

    #[test]
    fn lexer_errors() {
        assert_lex_err("\"unterminated");
        assert_lex_err("@");
        assert_lex_err("x = y");
        assert_lex_err("x =");
    }

    #[test]
    fn lexer_empty_string() {
        assert_lex("\"\"", &[Str("")]);
    }

    #[test]
    fn lexer_numbers_in_sequence() {
        assert_lex("123 456.78 9", &[Int(123), Float(456.78), Int(9)]);
    }

    #[test]
    fn lexer_trailing_operators() {
        assert_lex("!", &[Bang]);
        assert_lex("a <", &[Ident("a"), Less]);
        assert_lex(">", &[Greater]);
        assert_lex("|", &[Pipe]);
        assert_lex("&", &[Amp]);
        assert_lex_err("x =");
    }

    #[test]
    fn lexer_spans() {
        let src = "a != \"hi\" 3.14  ";
        let toks = lex(src).unwrap();

        assert_eq!(
            toks,
            vec![
                Token::new(Ident("a"), Span::new(0, 1)),
                Token::new(BangEqual, Span::new(2, 2)),
                Token::new(Str("hi"), Span::new(5, 4)),
                Token::new(Float(3.14), Span::new(10, 4)),
                Token::new(Eof, Span::new(16, 0)),
            ]
        );

        // The span covers the source text, quotes included
        assert_eq!(toks[1].span.slice(src), "!=");
        assert_eq!(toks[2].span.slice(src), "\"hi\"");
        assert_eq!(toks[3].span.slice(src), "3.14");
    }

    #[test]
    fn lexer_adjacent_operators() {
        assert_lex("!!", &[Bang, Bang]);
        assert_lex("<<", &[Less, Less]);
        assert_lex("!=!=", &[BangEqual, BangEqual]);
        assert_lex("<=>", &[LessEqual, Greater]);
        assert_lex("|||", &[PipePipe, Pipe]);
        assert_lex("&&&", &[AmpAmp, Amp]);

        // Greedy: "!==" is "!=" then a bare "=", never "!" then "=="
        assert_lex_err("!==");
        assert_lex_err(">==");
        assert_lex_err("===");
    }

    #[test]
    fn lexer_number_edges() {
        // A dot is only part of a number if a digit follows it
        assert_lex("1.", &[Int(1), Dot]);
        assert_lex("1.x", &[Int(1), Dot, Ident("x")]);
        assert_lex(".5", &[Dot, Int(5)]);
        assert_lex("1.2.3", &[Float(1.2), Dot, Int(3)]);

        // A number stops at the first non-digit
        assert_lex("123abc", &[Int(123), Ident("abc")]);
        assert_lex("1e5", &[Int(1), Ident("e5")]);

        // No negative literals: the sign is a separate token
        assert_lex("-5", &[Minus, Int(5)]);

        // Leading zeros are fine
        assert_lex("007", &[Int(7)]);

        // i32 bounds
        assert_lex("2147483647", &[Int(i32::MAX)]);
        assert_lex_err("2147483648");
    }

    #[test]
    fn lexer_empty_input() {
        assert_lex("", &[]);
        assert_lex(" ", &[]);
        assert_lex(" \t\r\n ", &[]);

        // Eof still lands at the end of the source, after the whitespace
        let toks = lex(" \t\r\n ").unwrap();
        assert_eq!(toks, vec![Token::new(Eof, Span::new(5, 0))]);
    }

    #[test]
    fn lexer_all_words() {
        use crate::tokens::Keyword as Kw;
        use crate::types::types::Prim as Pr;

        for &kw in Kw::ALL {
            let expected = match kw {
                // from_word checks booleans before keywords
                k => Keyword(k),
            };
            assert_lex(kw.as_str(), &[expected]);
        }

        for &p in Pr::ALL {
            assert_lex(p.as_str(), &[Prim(p)]);
        }
    }

    #[test]
    fn lexer_identifier_boundaries() {
        // Underscores anywhere in a name
        assert_lex("_foo", &[Ident("_foo")]);
        assert_lex("a_b_c", &[Ident("a_b_c")]);
        assert_lex("foo_", &[Ident("foo_")]);
        assert_lex("x1_y2", &[Ident("x1_y2")]);
        assert_lex("_", &[Ident("_")]);
        assert_lex("__", &[Ident("__")]);

        // A keyword or type with extra characters is just a name
        assert_lex("create_", &[Ident("create_")]);
        assert_lex("u32_x", &[Ident("u32_x")]);

        // A name ends at the first non-identifier character
        assert_lex("foo+bar", &[Ident("foo"), Plus, Ident("bar")]);
        assert_lex("foo.bar", &[Ident("foo"), Dot, Ident("bar")]);
    }

    #[test]
    fn lexer_non_ascii() {
        assert_lex_err("é");
        assert_lex_err("aé");
        assert_lex_err("a\u{00A0}b");

        assert_lex("\"héllo\"", &[Str("héllo")]);

        assert_lex_err("\"é");
    }

    #[test]
    fn lexer_string_contents() {
        assert_lex("\"a\nb\"", &[Str("a\nb")]);
        assert_lex("\"  \t \"", &[Str("  \t ")]);
        assert_lex("\"x = y @ !=\"", &[Str("x = y @ !=")]);

        assert_lex("\"a\\b\"", &[Str("a\\b")]);

        assert_lex_err("\"a\\\"b\"");

        assert_lex("\"a\"\"b\"", &[Str("a"), Str("b")]);
        assert_lex("\"a\"b", &[Str("a"), Ident("b")]);
    }
}
