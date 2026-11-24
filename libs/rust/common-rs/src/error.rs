#![allow(non_camel_case_types)]

use core::fmt::{self};

use crate::ffi::error::{err_t, error};

pub const SUCCESS: err_t = 0;
pub const CAUSE_MSG_LEN: usize = 256;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
#[repr(i32)]
pub enum ErrorCode {
    Io = -1,
    NoMem = -2,
    Arith = -3,
    Corrupt = -4,
    InvalidArgument = -5,
    PgOutOfRange = -6,
    Syntax = -7,
    Interp = -8,
    RptreePageStackOverflow = -9,
    DuplicateVariable = -10,
    VariableNotFound = -11,
    DuplicateCommit = -12,
    TooManyFiles = -14,
    TooManyThreads = -15,
    Crashed = -16,
}

pub type Result<T = ()> = core::result::Result<T, ErrorCode>;

impl ErrorCode {
    pub const fn from_raw(code: err_t) -> Option<Self> {
        Some(match code {
            c if c >= SUCCESS => return None,
            -1 => Self::Io,
            -2 => Self::NoMem,
            -3 => Self::Arith,
            -4 => Self::Corrupt,
            -5 => Self::InvalidArgument,
            -6 => Self::PgOutOfRange,
            -7 => Self::Syntax,
            -8 => Self::Interp,
            -9 => Self::RptreePageStackOverflow,
            -10 => Self::DuplicateVariable,
            -11 => Self::VariableNotFound,
            -12 => Self::DuplicateCommit,
            -14 => Self::TooManyFiles,
            -15 => Self::TooManyThreads,
            -16 => Self::Crashed,
            _ => unreachable!(),
        })
    }

    const fn jist(self) -> &'static str {
        match self {
            Self::Io => "I/O error",
            Self::NoMem => "out of memory",
            Self::Arith => "arithmetic overflow",
            Self::Corrupt => "corrupted data",
            Self::InvalidArgument => "invalid argument",
            Self::PgOutOfRange => "page out of range",
            Self::Syntax => "syntax error",
            Self::Interp => "interpreter error",
            Self::RptreePageStackOverflow => "rptree page stack overflow",
            Self::DuplicateVariable => "duplicate variable",
            Self::VariableNotFound => "variable does not exist",
            Self::DuplicateCommit => "duplicate commit",
            Self::TooManyFiles => "too many open files",
            Self::TooManyThreads => "too many threads",
            Self::Crashed => "crashed (debug build)",
        }
    }
}

impl fmt::Display for ErrorCode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str(self.jist())
    }
}

impl core::error::Error for ErrorCode {}

pub const fn check(ret: err_t, e: &mut Error) -> Result<()> {
    match ErrorCode::from_raw(ret) {
        Some(code) => Err(e.err_expect(code)),
        None => Ok(()),
    }
}

#[repr(transparent)]
pub struct Error(error);

impl Error {
    pub fn new() -> Self {
        Self(error {
            cause_code: SUCCESS,
            cause_msg: [0; CAUSE_MSG_LEN],
            cmlen: 0,
            disable_log: false,
        })
    }

    pub fn code(&self) -> Option<ErrorCode> {
        ErrorCode::from_raw(self.0.cause_code)
    }

    pub fn message(&self) -> &str {
        let len = self.msg_len();
        let ptr = self.0.cause_msg.as_ptr().cast::<u8>();
        let bytes = unsafe { core::slice::from_raw_parts(ptr, len) };
        match core::str::from_utf8(bytes) {
            Ok(s) => s,
            Err(e) => core::str::from_utf8(&bytes[..e.valid_up_to()]).unwrap_or(""),
        }
    }

    pub fn as_mut_ptr(&mut self) -> *mut error {
        assert!(self.0.cause_code == SUCCESS);
        &mut self.0
    }

    fn msg_len(&self) -> usize {
        (self.0.cmlen as usize).min(CAUSE_MSG_LEN - 1)
    }

    pub const fn err(&self) -> ErrorCode {
        assert!(self.0.cause_code < 0);
        ErrorCode::from_raw(self.0.cause_code).expect("only call failed on failed attempts")
    }

    pub const fn err_expect(&self, code: ErrorCode) -> ErrorCode {
        assert!(self.0.cause_code < 0);
        assert!(self.0.cause_code == code as err_t);
        ErrorCode::from_raw(self.0.cause_code).expect("only call failed on failed attempts")
    }

    pub fn reset(&mut self) {
        self.0.cause_code = SUCCESS;
    }
}

impl Default for Error {
    fn default() -> Self {
        Self::new()
    }
}

impl fmt::Debug for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Error")
            .field("code", &self.code())
            .field("message", &self.message())
            .finish()
    }
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match (self.code(), self.message()) {
            (None, _) => f.write_str("no error"),
            (Some(c), "") => write!(f, "{c}"),
            (Some(c), m) => write!(f, "{c}: {m}"),
        }
    }
}
