pub struct Txn<'db> {
    db: &'db Handle,
    raw: NonNull<sys::txn_t>,
}

impl Txn<'_> {
    pub fn commit(self) -> Result<()> {
        let this = ManuallyDrop::new(self);
        this.db.check(unsafe { sys::ns_commit(this.db.ptr(), this.raw.as_ptr()) })
    }

    pub fn rollback(self) -> Result<()> {
        let this = ManuallyDrop::new(self);
        this.db.check(unsafe { sys::ns_rollback(this.db.ptr(), this.raw.as_ptr()) })
    }

    pub fn as_raw(&self) -> *mut sys::txn_t {
        self.raw.as_ptr()
    }
}

impl Drop for Txn<'_> {
    fn drop(&mut self) {
        unsafe { sys::ns_rollback(self.db.ptr(), self.raw.as_ptr()) };
    }
}

