use core::{
    ffi::{c_char, c_int, c_void, CStr},
    marker::PhantomData,
};

use crate::{
    error::{check, Error, Result},
    ffi::{
        error::{err_t, error},
        os::{
            faulty_num_os_calls, faulty_os_create, faulty_os_fail_after, faulty_os_free,
            faulty_os_reset, faulty_os_set_fail, i_file, i_os, system_os_create, system_os_free,
        },
    },
    file::File,
    memory::Mem,
    utils::create,
};

pub struct Os<'a> {
    pub raw: i_os,
    is_faulty: bool,
    _deps: PhantomData<&'a ()>,
}

// A function to open a file
type OpenFn = unsafe extern "C" fn(*mut c_void, *mut i_file, *const c_char, *mut error) -> err_t;

impl<'a> Os<'a> {
    pub fn system(mem: &'a Mem, e: &mut Error) -> Result<Self> {
        let raw = create(e, |d, ep| unsafe { system_os_create(mem.raw, d, ep) })?;
        Ok(Self {
            raw,
            is_faulty: false,
            _deps: PhantomData,
        })
    }

    pub fn faulty(mem: &'a Mem, delegate: &'a Os<'a>, e: &mut Error) -> Result<Self> {
        let raw = create(e, |d, ep| unsafe {
            faulty_os_create(mem.raw, delegate.raw, 0.0, d, ep)
        })?;

        Ok(Self {
            raw,
            is_faulty: true,
            _deps: PhantomData,
        })
    }

    fn open_with(&self, open: OpenFn, path: &CStr, e: &mut Error) -> Result<File<'_>> {
        let raw = create(e, |d, ep| unsafe {
            open(self.raw.self_, d, path.as_ptr(), ep)
        })?;
        Ok(File { raw, os: self })
    }

    pub fn open_rw(&self, path: &CStr, e: &mut Error) -> Result<File<'_>> {
        self.open_with(self.raw.vt().open_file_rw, path, e)
    }

    pub fn open_r(&self, path: &CStr, e: &mut Error) -> Result<File<'_>> {
        self.open_with(self.raw.vt().open_file_r, path, e)
    }

    pub fn open_w(&self, path: &CStr, e: &mut Error) -> Result<File<'_>> {
        self.open_with(self.raw.vt().open_file_w, path, e)
    }

    /// Remove a file; does not fail if it doesn't exist.
    pub fn remove_quiet(&self, path: &CStr, e: &mut Error) -> Result<()> {
        let ret =
            unsafe { (self.raw.vt().remove_quiet)(self.raw.self_, path.as_ptr(), e.as_mut_ptr()) };
        check(ret, e)
    }

    pub fn unlink(&self, path: &CStr, e: &mut Error) -> Result<()> {
        let ret = unsafe { (self.raw.vt().unlink)(self.raw.self_, path.as_ptr(), e.as_mut_ptr()) };
        check(ret, e)
    }

    pub fn file_exists(&self, path: &CStr, e: &mut Error) -> Result<bool> {
        let mut exists = false;
        let ret = unsafe {
            (self.raw.vt().file_exists)(self.raw.self_, path.as_ptr(), &mut exists, e.as_mut_ptr())
        };
        check(ret, e)?;
        Ok(exists)
    }

    // fault injection

    pub fn faulty_set_percent(&self, fail_percent: f32) {
        assert!(self.is_faulty);
        unsafe { faulty_os_set_fail(self.raw, fail_percent) }
    }

    pub fn faulty_fail_after(&self, count: c_int) {
        assert!(self.is_faulty);
        unsafe { faulty_os_fail_after(self.raw, count) }
    }

    pub fn faulty_reset(&self) {
        assert!(self.is_faulty);
        unsafe { faulty_os_reset(self.raw) }
    }

    pub fn faulty_calls(&self) -> u32 {
        assert!(self.is_faulty);
        unsafe { faulty_num_os_calls(self.raw) }
    }
}

impl Drop for Os<'_> {
    fn drop(&mut self) {
        unsafe {
            if self.is_faulty {
                faulty_os_free(self.raw)
            } else {
                system_os_free(self.raw)
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // construction

    #[test]
    fn system_is_not_faulty() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        assert!(!os.is_faulty);
        Ok(())
    }

    #[test]
    fn faulty_is_marked_faulty() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let sys = Os::system(&mem, &mut e)?;
        let os = Os::faulty(&mem, &sys, &mut e)?;
        assert!(os.is_faulty);
        Ok(())
    }

    // file_exists

    #[test]
    fn file_exists_false_for_missing_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/os_test_exists_missing.tmp";
        os.remove_quiet(path, &mut e)?;
        assert!(!os.file_exists(path, &mut e)?);
        Ok(())
    }

    #[test]
    fn file_exists_true_after_creation() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/os_test_exists_present.tmp";
        os.remove_quiet(path, &mut e)?;

        drop(os.open_w(path, &mut e)?);
        assert!(os.file_exists(path, &mut e)?);

        os.remove_quiet(path, &mut e)?;
        Ok(())
    }

    // open_w / open_r / open_rw

    #[test]
    fn open_w_creates_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/os_test_open_w.tmp";
        os.remove_quiet(path, &mut e)?;
        assert!(!os.file_exists(path, &mut e)?);

        {
            let _f = os.open_w(path, &mut e)?;
        }
        assert!(os.file_exists(path, &mut e)?);

        os.remove_quiet(path, &mut e)?;
        Ok(())
    }

    #[test]
    fn open_r_succeeds_on_existing_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/os_test_open_r_existing.tmp";
        os.remove_quiet(path, &mut e)?;
        drop(os.open_w(path, &mut e)?);

        assert!(os.open_r(path, &mut e).is_ok());

        os.remove_quiet(path, &mut e)?;
        Ok(())
    }

    #[test]
    fn open_rw_succeeds_on_existing_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/os_test_open_rw_existing.tmp";
        os.remove_quiet(path, &mut e)?;
        drop(os.open_w(path, &mut e)?);

        assert!(os.open_rw(path, &mut e).is_ok());

        os.remove_quiet(path, &mut e)?;
        Ok(())
    }

    // remove_quiet

    #[test]
    fn remove_quiet_removes_existing_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/os_test_remove_quiet_existing.tmp";
        drop(os.open_w(path, &mut e)?);
        assert!(os.file_exists(path, &mut e)?);

        os.remove_quiet(path, &mut e)?;
        assert!(!os.file_exists(path, &mut e)?);
        Ok(())
    }

    #[test]
    fn remove_quiet_ok_on_missing_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/os_test_remove_quiet_missing.tmp";
        os.remove_quiet(path, &mut e)?;
        os.remove_quiet(path, &mut e)?; // twice, still fine
        Ok(())
    }

    // unlink

    #[test]
    fn unlink_removes_existing_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/os_test_unlink_existing.tmp";
        os.remove_quiet(path, &mut e)?;
        drop(os.open_w(path, &mut e)?);

        os.unlink(path, &mut e)?;
        assert!(!os.file_exists(path, &mut e)?);
        Ok(())
    }

    #[test]
    fn unlink_fails_on_missing_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/os_test_unlink_missing.tmp";
        os.remove_quiet(path, &mut e)?;
        assert!(os.unlink(path, &mut e).is_err());
        Ok(())
    }

    // fault injection: failures propagate

    #[test]
    fn faulty_fail_after_zero_fails_every_operation() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let sys = Os::system(&mem, &mut e)?;
        let os = Os::faulty(&mem, &sys, &mut e)?;
        let path = c"/tmp/os_test_faulty_ops.tmp";

        os.faulty_fail_after(0);
        assert!(os.open_w(path, &mut e).is_err());
        e.reset();
        os.faulty_fail_after(0);
        assert!(os.open_r(path, &mut e).is_err());
        e.reset();
        os.faulty_fail_after(0);
        assert!(os.open_rw(path, &mut e).is_err());
        e.reset();
        os.faulty_fail_after(0);
        assert!(os.remove_quiet(path, &mut e).is_err());
        e.reset();
        Ok(())
    }

    #[test]
    fn faulty_failed_open_w_does_not_create_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let sys = Os::system(&mem, &mut e)?;
        let os = Os::faulty(&mem, &sys, &mut e)?;
        let path = c"/tmp/os_test_faulty_no_create.tmp";
        sys.remove_quiet(path, &mut e)?;

        os.faulty_fail_after(0);
        assert!(os.open_w(path, &mut e).is_err());
        e.reset();
        assert!(!sys.file_exists(path, &mut e)?);
        Ok(())
    }

    // faulty_calls

    #[test]
    fn faulty_calls_starts_at_zero() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let sys = Os::system(&mem, &mut e)?;
        let os = Os::faulty(&mem, &sys, &mut e)?;
        assert_eq!(os.faulty_calls(), 0);
        Ok(())
    }

    // non-faulty guard assertions

    #[test]
    #[should_panic]
    fn faulty_set_percent_panics_on_system_os() {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e).ok().unwrap_or_else(|| panic!());
        os.faulty_set_percent(0.5);
    }

    #[test]
    #[should_panic]
    fn faulty_fail_after_panics_on_system_os() {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e).ok().unwrap_or_else(|| panic!());
        os.faulty_fail_after(1);
    }

    #[test]
    #[should_panic]
    fn faulty_reset_panics_on_system_os() {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e).ok().unwrap_or_else(|| panic!());
        os.faulty_reset();
    }

    #[test]
    #[should_panic]
    fn faulty_calls_panics_on_system_os() {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e).ok().unwrap_or_else(|| panic!());
        let _ = os.faulty_calls();
    }

    // Drop

    #[test]
    fn drop_system_and_faulty_do_not_crash() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let sys = Os::system(&mem, &mut e)?;
        {
            let _faulty = Os::faulty(&mem, &sys, &mut e)?;
        } // faulty freed before its delegate
        Ok(())
    }
}
