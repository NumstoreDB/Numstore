pub struct Var {
    raw: NonNull<sys::nsdb_var_t>,
}

impl Var {
    /// Length in elements (not bytes).
    pub fn len(&self) -> u64 {
        unsafe { sys::ns_var_len(self.raw.as_ptr()) }
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    pub fn as_raw(&self) -> *mut sys::nsdb_var_t {
        self.raw.as_ptr()
    }
}

impl Drop for Var {
    fn drop(&mut self) {
        unsafe { sys::ns_var_free(self.raw.as_ptr()) };
    }
}

impl fmt::Debug for Var {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("Var").field("len", &self.len()).finish()
    }
}

