const FMT_S: *const c_char = c"%s".as_ptr();

pub struct Database {
    h: Handle,
}

impl Database {
    pub fn open(path: impl AsRef<Path>) -> Result<Self> {
        let p = cpath(path.as_ref())?;
        Ok(Database { h: Handle::from_raw(unsafe { sys::ns_open(p.as_ptr()) }, "ns_open")? })
    }

    /// Close gracefully, reporting any error (drop also closes, silently).
    pub fn close(self) -> Result<()> {
        self.h.close()
    }

    /// Close harshly; incomplete transactions roll back on the next open.
    pub fn crash(self) -> Result<()> {
        self.h.crash()
    }

    pub fn begin(&self) -> Result<Txn<'_>> {
        self.h.begin()
    }

    pub fn as_raw(&self) -> *mut sys::nsdb_t {
        self.h.ptr()
    }

    /// Run a query that takes no data, e.g. `"create foo u32"`.
    pub fn execute(&self, txn: &Txn<'_>, query: &str) -> Result<()> {
        let tx = self.h.txn(txn)?;
        let q = cstr(query)?;
        self.h.check(unsafe { sys::ns_execute(self.h.ptr(), tx, FMT_S, q.as_ptr()) })
    }

    /// Get the variable a query refers to, e.g. `"get foo"`.
    pub fn get_var(&self, txn: &Txn<'_>, query: &str) -> Result<Var> {
        let tx = self.h.txn(txn)?;
        let q = cstr(query)?;
        let raw = unsafe { sys::ns_get_var(self.h.ptr(), tx, FMT_S, q.as_ptr()) };
        NonNull::new(raw).map(|raw| Var { raw }).ok_or_else(|| self.h.error(-1))
    }

    /// READ / REMOVE query into a byte buffer. Returns the count numstore reports.
    pub fn read_bytes(&self, txn: &Txn<'_>, dest: &mut [u8], query: &str) -> Result<u64> {
        let tx = self.h.txn(txn)?;
        let q = cstr(query)?;
        let rc = unsafe {
            sys::ns_read(
                self.h.ptr(),
                tx,
                dest.as_mut_ptr().cast(),
                dest.len() as b_size,
                FMT_S,
                q.as_ptr(),
            )
        };
        self.h.check_sb(rc)
    }

    /// Typed [`read_bytes`](Self::read_bytes).
    pub fn read<T: Element>(&self, txn: &Txn<'_>, dest: &mut [T], query: &str) -> Result<u64> {
        self.read_bytes(txn, as_bytes_mut(dest), query)
    }

    /// INSERT / WRITE query from a byte buffer.
    pub fn write_bytes(&self, txn: &Txn<'_>, src: &[u8], query: &str) -> Result<u64> {
        let tx = self.h.txn(txn)?;
        let q = cstr(query)?;
        let rc = unsafe {
            sys::ns_write(
                self.h.ptr(),
                tx,
                src.as_ptr().cast(),
                src.len() as b_size,
                FMT_S,
                q.as_ptr(),
            )
        };
        self.h.check_sb(rc)
    }

    /// Typed [`write_bytes`](Self::write_bytes).
    pub fn write<T: Element>(&self, txn: &Txn<'_>, src: &[T], query: &str) -> Result<u64> {
        self.write_bytes(txn, as_bytes(src), query)
    }

    /// READ / REMOVE query into a library-allocated buffer.
    pub fn read_alloc(&self, txn: &Txn<'_>, query: &str) -> Result<MallocBuf> {
        let tx = self.h.txn(txn)?;
        let q = cstr(query)?;
        let mut dlen = DLEN_SENTINEL;
        let p = unsafe { sys::ns_malloc(self.h.ptr(), tx, &mut dlen, FMT_S, q.as_ptr()) };
        wrap_malloc(p, dlen, || self.h.error(-1))
    }

    /// Prepare a query plan. The plan borrows the database, so it is always
    /// freed before the database is closed.
    pub fn plan(&self, query: &str) -> Result<Plan<'_>> {
        let q = cstr(query)?;
        let raw = unsafe { sys::ns_plan_fcreate(self.h.ptr(), FMT_S, q.as_ptr()) };
        NonNull::new(raw)
            .map(|raw| Plan { db: &self.h, raw, _p: PhantomData })
            .ok_or_else(|| self.h.error(-1))
    }
}
