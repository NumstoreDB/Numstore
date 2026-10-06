pub struct Plan<'db> {
    db: &'db Handle,
    raw: NonNull<sys::nsdb_plan_t>,
    _p: PhantomData<&'db Database>,
}

impl Plan<'_> {
    fn error(&self, code: i64) -> Error {
        let message = unsafe { take_msg(sys::ns_plan_strerror(self.raw.as_ptr())) };
        Error::Numstore { code, message }
    }

    fn check_sb(&self, rc: sb_size) -> Result<u64> {
        if rc < 0 { Err(self.error(rc)) } else { Ok(rc as u64) }
    }

    pub fn as_raw(&self) -> *mut sys::nsdb_plan_t {
        self.raw.as_ptr()
    }

    pub fn execute(&self, txn: &Txn<'_>) -> Result<()> {
        let tx = self.db.txn(txn)?;
        let rc = unsafe { sys::ns_plan_execute(self.raw.as_ptr(), tx) };
        if rc < 0 { Err(self.error(rc.into())) } else { Ok(()) }
    }

    pub fn get_var(&self, txn: &Txn<'_>) -> Result<Var> {
        let tx = self.db.txn(txn)?;
        let raw = unsafe { sys::ns_plan_get_var(self.raw.as_ptr(), tx) };
        NonNull::new(raw).map(|raw| Var { raw }).ok_or_else(|| self.error(-1))
    }

    pub fn read_bytes(&self, txn: &Txn<'_>, dest: &mut [u8]) -> Result<u64> {
        let tx = self.db.txn(txn)?;
        let rc = unsafe {
            sys::ns_plan_read(self.raw.as_ptr(), tx, dest.as_mut_ptr().cast(), dest.len() as b_size)
        };
        self.check_sb(rc)
    }

    pub fn read<T: Element>(&self, txn: &Txn<'_>, dest: &mut [T]) -> Result<u64> {
        self.read_bytes(txn, as_bytes_mut(dest))
    }

    pub fn write_bytes(&self, txn: &Txn<'_>, src: &[u8]) -> Result<u64> {
        let tx = self.db.txn(txn)?;
        let rc = unsafe {
            sys::ns_plan_write(self.raw.as_ptr(), tx, src.as_ptr().cast(), src.len() as b_size)
        };
        self.check_sb(rc)
    }

    pub fn write<T: Element>(&self, txn: &Txn<'_>, src: &[T]) -> Result<u64> {
        self.write_bytes(txn, as_bytes(src))
    }

    pub fn read_alloc(&self, txn: &Txn<'_>) -> Result<MallocBuf> {
        let tx = self.db.txn(txn)?;
        let mut dlen = DLEN_SENTINEL;
        let p = unsafe { sys::ns_plan_malloc(self.raw.as_ptr(), tx, &mut dlen) };
        wrap_malloc(p, dlen, || self.error(-1))
    }
}

impl Drop for Plan<'_> {
    fn drop(&mut self) {
        unsafe { sys::ns_plan_free(self.raw.as_ptr()) };
    }
}

