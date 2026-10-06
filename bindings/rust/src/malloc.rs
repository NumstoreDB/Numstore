// The system allocator's free, needed for ns_malloc / ns_plan_malloc.
extern "C" {
    #[link_name = "free"]
    pub fn libc_free(p: *mut c_void);
}

pub struct MallocBuf {
    ptr: *mut u8,
    len: usize,
}

impl MallocBuf {
    /// Copy into a `Vec<T>`. Returns `None` if the length isn't a multiple
    /// of `size_of::<T>()`.
    pub fn to_vec_of<T: Element>(&self) -> Option<Vec<T>> {
        let sz = size_of::<T>();
        if sz == 0 || self.len % sz != 0 {
            return None;
        }
        let n = self.len / sz;
        let mut v = Vec::<T>::with_capacity(n);
        // SAFETY: Element allows any bit pattern; the copy is unaligned-safe.
        unsafe {
            ptr::copy_nonoverlapping(self.ptr, v.as_mut_ptr().cast::<u8>(), self.len);
            v.set_len(n);
        }
        Some(v)
    }
}

impl Deref for MallocBuf {
    type Target = [u8];
    fn deref(&self) -> &[u8] {
        if self.ptr.is_null() {
            &[]
        } else {
            unsafe { std::slice::from_raw_parts(self.ptr, self.len) }
        }
    }
}

impl Drop for MallocBuf {
    fn drop(&mut self) {
        if !self.ptr.is_null() {
            unsafe { sys::libc_free(self.ptr.cast()) };
        }
    }
}

impl fmt::Debug for MallocBuf {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("MallocBuf").field("len", &self.len).finish()
    }
}

fn wrap_malloc(p: *mut c_void, dlen: b_size, err: impl FnOnce() -> Error) -> Result<MallocBuf> {
    if p.is_null() && dlen != 0 {
        return Err(err());
    }
    Ok(MallocBuf { ptr: p.cast(), len: dlen as usize })
}

const DLEN_SENTINEL: b_size = b_size::MAX;
