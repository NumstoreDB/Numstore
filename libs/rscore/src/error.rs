use std::{collections::TryReserveError, fmt};

#[derive(Debug, Clone, PartialEq)]
pub enum Error {
    InvalidType(String),
    Corrupt(String),
    Arith(String),
    NoMem,
    InvalidArgument,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
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
