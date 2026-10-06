pub struct SmartFile {
    h: Handle,
}

impl SmartFile {
    pub fn open(path: impl AsRef<Path>) -> Result<Self> {
        let p = cpath(path.as_ref())?;
        Ok(SmartFile {
            h: Handle::from_raw(unsafe { sys::ns_smfile_open(p.as_ptr()) }, "ns_smfile_open")?,
        })
    }

    pub fn close(self) -> Result<()> {
        self.h.close()
    }

    pub fn crash(self) -> Result<()> {
        self.h.crash()
    }

    pub fn begin(&self) -> Result<Txn<'_>> {
        self.h.begin()
    }

    pub fn as_raw(&self) -> *mut sys::nsdb_t {
        self.h.ptr()
    }

    /// Size in bytes as seen by `txn`.
    pub fn size(&self, txn: &Txn<'_>) -> Result<u64> {
        let tx = self.h.txn(txn)?;
        self.h.check_sb(unsafe { sys::ns_smfile_size(self.h.ptr(), tx) })
    }

    /// Insert raw bytes at byte offset `bofst`.
    pub fn insert_bytes(&self, txn: &Txn<'_>, src: &[u8], bofst: sb_size) -> Result<u64> {
        let tx = self.h.txn(txn)?;
        let rc = unsafe {
            sys::ns_smfile_insert(self.h.ptr(), tx, src.as_ptr().cast(), bofst, src.len() as b_size)
        };
        self.h.check_sb(rc)
    }

    /// Insert typed elements at byte offset `bofst`.
    pub fn insert<T: Element>(&self, txn: &Txn<'_>, src: &[T], bofst: sb_size) -> Result<u64> {
        self.insert_bytes(txn, as_bytes(src), bofst)
    }

    /// Overwrite `src.len()` elements starting at byte offset `bofst`,
    /// stepping `stride` elements between writes.
    pub fn write<T: Element>(
        &self,
        txn: &Txn<'_>,
        src: &[T],
        bofst: sb_size,
        stride: sb_size,
    ) -> Result<u64> {
        let tx = self.h.txn(txn)?;
        let rc = unsafe {
            sys::ns_smfile_write(
                self.h.ptr(),
                tx,
                src.as_ptr().cast(),
                elem_size::<T>()?,
                bofst,
                stride,
                src.len() as b_size,
            )
        };
        self.h.check_sb(rc)
    }

    /// Read `dest.len()` elements starting at byte offset `bofst`,
    /// stepping `stride` elements between reads.
    pub fn read<T: Element>(
        &self,
        txn: &Txn<'_>,
        dest: &mut [T],
        bofst: sb_size,
        stride: sb_size,
    ) -> Result<u64> {
        let tx = self.h.txn(txn)?;
        let rc = unsafe {
            sys::ns_smfile_read(
                self.h.ptr(),
                tx,
                dest.as_mut_ptr().cast(),
                elem_size::<T>()?,
                bofst,
                stride,
                dest.len() as b_size,
            )
        };
        self.h.check_sb(rc)
    }

    /// Remove `dest.len()` elements and copy them into `dest`.
    pub fn remove_into<T: Element>(
        &self,
        txn: &Txn<'_>,
        dest: &mut [T],
        bofst: sb_size,
        stride: sb_size,
    ) -> Result<u64> {
        let tx = self.h.txn(txn)?;
        let rc = unsafe {
            sys::ns_smfile_remove(
                self.h.ptr(),
                tx,
                dest.as_mut_ptr().cast(),
                elem_size::<T>()?,
                bofst,
                stride,
                dest.len() as b_size,
            )
        };
        self.h.check_sb(rc)
    }

    /// Remove `nelem` elements of type `T` without keeping them
    /// (passes NULL as `dest`).
    pub fn remove<T: Element>(
        &self,
        txn: &Txn<'_>,
        bofst: sb_size,
        stride: sb_size,
        nelem: b_size,
    ) -> Result<u64> {
        let tx = self.h.txn(txn)?;
        let rc = unsafe {
            sys::ns_smfile_remove(
                self.h.ptr(),
                tx,
                ptr::null_mut(),
                elem_size::<T>()?,
                bofst,
                stride,
                nelem,
            )
        };
        self.h.check_sb(rc)
    }
}

