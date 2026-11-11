use std::fmt::Write as _;
use std::{collections::TryReserveError, fmt};

use crate::tokens::Token;

// Puts vertical bars around the section of interest
// in the string [ctx]
fn write_error(
    f: &mut fmt::Formatter<'_>,
    message: &str,
    unit: &str,
    at: usize,
    ctx: &str,
    ctx_start: usize,
    ctx_pos: usize,
) -> fmt::Result {
    let s = ctx_start.min(ctx.len());
    let e = ctx_pos.clamp(s, ctx.len());
    write!(
        f,
        "error: {message} at {unit} {at}: {}|{}|{}",
        &ctx[..s],
        &ctx[s..e],
        &ctx[e..]
    )
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LexerError {
    message: String,

    start: usize,
    pos: usize,

    context: String,
    ctx_start: usize,
    ctx_pos: usize,
}

impl LexerError {
    /// Bytes of context to keep on each side of the error span.
    const CONTEXT_RADIUS: usize = 20;

    pub fn new(message: impl Into<String>, src: &str, start: usize, pos: usize) -> Self {
        let pos = pos.min(src.len());
        let start = start.min(pos);

        // Stay on the error's line...
        let line_start = src[..start].rfind('\n').map_or(0, |i| i + 1);
        let line_end = src[pos..].find('\n').map_or(src.len(), |i| pos + i);

        // ...and keep at most CONTEXT_RADIUS bytes either side, without
        // splitting a multi-byte character.
        let mut ctx_lo = start.saturating_sub(Self::CONTEXT_RADIUS).max(line_start);
        while !src.is_char_boundary(ctx_lo) {
            ctx_lo += 1;
        }
        let mut ctx_hi = (pos + Self::CONTEXT_RADIUS).min(line_end);
        while !src.is_char_boundary(ctx_hi) {
            ctx_hi -= 1;
        }

        Self {
            message: message.into(),
            start,
            pos,
            context: src[ctx_lo..ctx_hi].to_owned(),
            ctx_start: start - ctx_lo,
            ctx_pos: pos - ctx_lo,
        }
    }
}

impl fmt::Display for LexerError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write_error(
            f,
            &self.message,
            "byte",
            self.start,
            &self.context,
            self.ctx_start,
            self.ctx_pos,
        )
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ParserError {
    pub message: String,

    /// Index of the offending token.
    pub pos: usize,

    /// Rendered tokens around the error.
    pub context: String,
    /// Byte range of the offending token within `context`.
    pub ctx_start: usize,
    pub ctx_pos: usize,
}

impl ParserError {
    /// Tokens of context to keep on each side of the error.
    const CONTEXT_RADIUS: usize = 5;

    pub fn new(message: impl Into<String>, tokens: &[Token<'_>], pos: usize) -> Self {
        let pos = pos.min(tokens.len());
        let lo = pos.saturating_sub(Self::CONTEXT_RADIUS);
        let hi = (pos + 1 + Self::CONTEXT_RADIUS).min(tokens.len());

        let mut context = String::new();
        // Defaults cover pos == tokens.len(): an empty marker at the end.
        let (mut ctx_start, mut ctx_pos) = (usize::MAX, usize::MAX);

        for (i, tok) in tokens[lo..hi].iter().enumerate().map(|(j, t)| (lo + j, t)) {
            if i > lo {
                context.push(' ');
            }
            if i == pos {
                ctx_start = context.len();
            }
            let _ = write!(context, "{tok:?}");
            if i == pos {
                ctx_pos = context.len();
            }
        }

        if ctx_start == usize::MAX {
            ctx_start = context.len();
            ctx_pos = context.len();
        }

        Self {
            message: message.into(),
            pos,
            context,
            ctx_start,
            ctx_pos,
        }
    }
}

impl fmt::Display for ParserError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write_error(
            f,
            &self.message,
            "token",
            self.pos,
            &self.context,
            self.ctx_start,
            self.ctx_pos,
        )
    }
}

#[derive(Debug, Clone, PartialEq)]
pub enum Error {
    Lex(LexerError),
    Parse(ParserError),
    InvalidType(String),
    Corrupt(String),
    Arith(String),
    NoMem,
    InvalidArgument,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::Lex(e) => write!(f, "{e}"),
            Error::Parse(e) => write!(f, "{e}"),
            Error::InvalidType(e) | Error::Corrupt(e) | Error::Arith(e) => write!(f, "{e}"),
            Error::NoMem => f.write_str("out of memory"),
            Error::InvalidArgument => f.write_str("invalid argument"),
        }
    }
}

impl From<TryReserveError> for Error {
    fn from(_: TryReserveError) -> Self {
        Error::NoMem
    }
}

impl std::error::Error for Error {}

pub type Result<T> = std::result::Result<T, Error>;
