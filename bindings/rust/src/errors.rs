use std::ffi::{c_char, c_int, c_void, CStr, CString};
use std::fmt;
use std::marker::PhantomData;
use std::mem::{size_of, size_of_val, ManuallyDrop};
use std::ops::Deref;
use std::path::Path;
use std::ptr::{self, NonNull};

#[allow(non_camel_case_types)]
pub mod sys {
}

pub use sys::{b_size, sb_size, t_size, NS_END, SMF_END};

// ===========================================================================
// Errors
// ===========================================================================

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Error {
    /// numstore reported a failure. `message` comes from `ns_strerror` /
    /// `ns_plan_strerror`; `code` is the raw return value (or -1 for a NULL
    /// pointer return).
    Numstore { code: i64, message: String },
    /// A query or path contained an interior NUL byte.
    InteriorNul,
    /// A path was not valid for the platform's C string encoding.
    InvalidPath,
    /// A transaction was used with a different database than it came from.
    WrongDatabase,
    /// An element type is too large to describe with `t_size`.
    ElementTooLarge,
}

impl fmt::Display for Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Error::Numstore { code, message } => write!(f, "numstore error {code}: {message}"),
            Error::InteriorNul => f.write_str("string contains an interior NUL byte"),
            Error::InvalidPath => f.write_str("path cannot be represented as a C string"),
            Error::WrongDatabase => f.write_str("transaction belongs to a different database"),
            Error::ElementTooLarge => f.write_str("element size exceeds t_size"),
        }
    }
}

impl std::error::Error for Error {}

pub type Result<T> = std::result::Result<T, Error>;

unsafe fn take_msg(p: *const c_char) -> String {
    if p.is_null() {
        "unknown error".to_owned()
    } else {
        CStr::from_ptr(p).to_string_lossy().into_owned()
    }
}

fn cstr(s: &str) -> Result<CString> {
    CString::new(s).map_err(|_| Error::InteriorNul)
}

fn cpath(p: &Path) -> Result<CString> {
    #[cfg(unix)]
    {
        use std::os::unix::ffi::OsStrExt;
        CString::new(p.as_os_str().as_bytes()).map_err(|_| Error::InteriorNul)
    }
    #[cfg(not(unix))]
    {
        cstr(p.to_str().ok_or(Error::InvalidPath)?)
    }
}
