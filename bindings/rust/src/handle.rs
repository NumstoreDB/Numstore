struct Handle {
    raw: NonNull<sys::nsdb_t>,
}

impl Handle {
    fn from_raw(raw: *mut sys::nsdb_t, what: &str) -> Result<Self> {
        NonNull::new(raw).map(|raw| Handle { raw }).ok_or_else(|| Error::Numstore {
            code: -1,
            message: format!("{what} failed"),
        })
    }

    fn ptr(&self) -> *mut sys::nsdb_t {
        self.raw.as_ptr()
    }

    fn error(&self, code: i64) -> Error {
        let message = unsafe { take_msg(sys::ns_strerror(self.ptr())) };
        Error::Numstore { code, message }
    }

    fn check(&self, rc: c_int) -> Result<()> {
        if rc < 0 { Err(self.error(rc.into())) } else { Ok(()) }
    }

    fn check_sb(&self, rc: sb_size) -> Result<u64> {
        if rc < 0 { Err(self.error(rc)) } else { Ok(rc as u64) }
    }

    fn begin(&self) -> Result<Txn<'_>> {
        let raw = unsafe { sys::ns_begin(self.ptr()) };
        match NonNull::new(raw) {
            Some(raw) => Ok(Txn { db: self, raw }),
            None => Err(self.error(-1)),
        }
    }

    fn txn(&self, txn: &Txn<'_>) -> Result<*mut sys::txn_t> {
        if txn.db.raw == self.raw { Ok(txn.raw.as_ptr()) } else { Err(Error::WrongDatabase) }
    }

    fn close(self) -> Result<()> {
        let this = ManuallyDrop::new(self);
        let rc = unsafe { sys::ns_close(this.ptr()) };
        // The handle is gone either way; we can't call ns_strerror on it.
        if rc < 0 {
            Err(Error::Numstore { code: rc.into(), message: "ns_close failed".into() })
        } else {
            Ok(())
        }
    }

    fn crash(self) -> Result<()> {
        let this = ManuallyDrop::new(self);
        let rc = unsafe { sys::ns_crash(this.ptr()) };
        if rc < 0 {
            Err(Error::Numstore { code: rc.into(), message: "ns_crash failed".into() })
        } else {
            Ok(())
        }
    }
}

impl Drop for Handle {
    fn drop(&mut self) {
        unsafe { sys::ns_close(self.ptr()) };
    }
}

/// Remove a numstore database or smart file at `path`.
pub fn cleanup(path: impl AsRef<Path>) -> Result<()> {
    let p = cpath(path.as_ref())?;
    let rc = unsafe { sys::ns_cleanup(p.as_ptr()) };
    if rc < 0 {
        Err(Error::Numstore { code: rc.into(), message: "ns_cleanup failed".into() })
    } else {
        Ok(())
    }
}
