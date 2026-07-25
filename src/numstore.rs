use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_void};
use std::ptr::NonNull;

// =============================================================================
// SECTION: Unsafe C Bindings
// =============================================================================

#[repr(C)]
struct nsdb_t {
    _private: [u8; 0],
}

unsafe extern "C" {
    fn nsdb_open(path: *const c_char) -> *mut nsdb_t;
    fn nsdb_close(ns: *mut nsdb_t) -> i32;
    fn nsdb_cleanup(path: *const c_char) -> i32;
    fn nsdb_crash(ns: *mut nsdb_t) -> i32;

    fn nsdb_strerror(ns: *mut nsdb_t) -> *const c_char;

    fn nsdb_begin(ns: *mut nsdb_t) -> i32;
    fn nsdb_commit(ns: *mut nsdb_t) -> i32;
    fn nsdb_rollback(ns: *mut nsdb_t) -> i32;

    fn nsdb_execute(ns: *mut nsdb_t, query: *const c_char, data: *mut c_void) -> i64;
}

// =============================================================================
// SECTION: Safe Numstore Wrapper
// =============================================================================

pub struct NsDb {
    ptr: NonNull<nsdb_t>,
}

/// Errors returned from Numstore calls, carrying the message from
/// `nsdb_strerror` when one is available.
#[derive(Debug)]
pub struct NsError(pub String);

impl std::fmt::Display for NsError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "numstore error: {}", self.0)
    }
}
impl std::error::Error for NsError {}

type Result<T> = std::result::Result<T, NsError>;

impl NsDb {
    /// Opens (or creates) a database at `path`.
    pub fn open(path: &str) -> Result<Self> {
        let c_path = CString::new(path).map_err(|_| NsError("path contains NUL byte".into()))?;

        let raw = unsafe { nsdb_open(c_path.as_ptr()) };
        match NonNull::new(raw) {
            Some(ptr) => Ok(NsDb { ptr }),
            None => Err(NsError(format!("failed to open database at '{path}'"))),
        }
    }

    /// Deletes all resources associated with the database at `path`.
    /// Does not require an open handle.
    pub fn cleanup(path: &str) -> Result<()> {
        let c_path = CString::new(path).map_err(|_| NsError("path contains NUL byte".into()))?;
        let rc = unsafe { nsdb_cleanup(c_path.as_ptr()) };
        if rc < 0 {
            Err(NsError(format!("cleanup failed for '{path}'")))
        } else {
            Ok(())
        }
    }

    /// Simulates a crash, leaving the WAL in place so the next `open`
    /// enters recovery mode.
    pub fn crash(&mut self) -> Result<()> {
        let rc = unsafe { nsdb_crash(self.ptr.as_ptr()) };
        self.check(rc)
    }

    pub fn begin(&mut self) -> Result<()> {
        let rc = unsafe { nsdb_begin(self.ptr.as_ptr()) };
        self.check(rc)
    }

    pub fn commit(&mut self) -> Result<()> {
        let rc = unsafe { nsdb_commit(self.ptr.as_ptr()) };
        self.check(rc)
    }

    pub fn rollback(&mut self) -> Result<()> {
        let rc = unsafe { nsdb_rollback(self.ptr.as_ptr()) };
        self.check(rc)
    }

    /// Runs a query, optionally reading/writing through `data`.
    /// Returns the number of elements affected (for data ops) or 0.
    ///
    /// # Safety
    /// `data` must point to a buffer large enough and correctly typed for
    /// whatever the query does (insert/read/write/remove) — this is not
    /// checked on the Rust side, matching the C API's contract.
    pub unsafe fn execute(&mut self, query: &str, data: *mut c_void) -> Result<i64> {
        let c_query = CString::new(query).map_err(|_| NsError("query contains NUL byte".into()))?;

        let rc = unsafe { nsdb_execute(self.ptr.as_ptr(), c_query.as_ptr(), data) };
        if rc < 0 {
            Err(self.last_error())
        } else {
            Ok(rc)
        }
    }

    /// Convenience for queries that don't touch a data buffer
    /// (e.g. `create`, `delete if exists`).
    pub fn execute_stmt(&mut self, query: &str) -> Result<()> {
        unsafe { self.execute(query, std::ptr::null_mut()) }.map(|_| ())
    }

    fn check(&mut self, rc: i32) -> Result<()> {
        if rc < 0 {
            Err(self.last_error())
        } else {
            Ok(())
        }
    }

    fn last_error(&mut self) -> NsError {
        let msg_ptr = unsafe { nsdb_strerror(self.ptr.as_ptr()) };
        if msg_ptr.is_null() {
            NsError("unknown error".into())
        } else {
            let msg = unsafe { CStr::from_ptr(msg_ptr) }
                .to_string_lossy()
                .into_owned();
            NsError(msg)
        }
    }
}

impl Drop for NsDb {
    fn drop(&mut self) {
        unsafe {
            nsdb_close(self.ptr.as_ptr());
        }
    }
}

// Numstore handles aren't documented as thread-safe, so don't claim Send/Sync.
