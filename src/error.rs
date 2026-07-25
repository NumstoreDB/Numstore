use std::{io, net};

#[derive(Debug)]
pub enum IOErrorData {
    AddrParseError(net::AddrParseError),
    IOError(io::Error),
}

#[derive(Debug)]
pub enum Error {
    NetworkTimeout(String),
    IncompleteClientMessage,
    IncommingMessageTooLong,
    IOError(IOErrorData),
}
impl From<io::Error> for Error {
    fn from(value: io::Error) -> Self {
        Error::IOError(IOErrorData::IOError(value))
    }
}

impl From<net::AddrParseError> for Error {
    fn from(value: net::AddrParseError) -> Self {
        Error::IOError(IOErrorData::AddrParseError(value))
    }
}
