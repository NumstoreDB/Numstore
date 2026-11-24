#[derive(Debug)]
pub enum ErrorType {
    IoErr,
    InvalidArgument,
}

#[derive(Debug)]
pub struct Error {
    pub code: ErrorType,
    pub message: String,
}

pub type Result<T> = std::result::Result<T, Error>;

impl Error {
    pub fn new(code: ErrorType, message: impl Into<String>) -> Self {
        Self {
            code,
            message: message.into(),
        }
    }
}
