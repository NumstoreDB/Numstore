use crate::error::{Error, Result};
use crate::ffi::memory::{
    default_mem, faulty_mem_create, faulty_mem_fail_after, faulty_mem_free, faulty_mem_reset,
    faulty_mem_set_fail, faulty_num_mallocs, i_mem,
};
use crate::utils::create;
use core::ffi::{c_int, c_void};
use core::marker::PhantomData;
use core::{num::NonZeroU32, ptr::NonNull};

pub struct Mem<'a> {
    pub raw: i_mem,
    is_faulty: bool,
    delegate_lifetime: PhantomData<&'a ()>,
}

impl<'a> Mem<'a> {
    /// Create default system memory instance
    pub fn system() -> Self {
        Self {
            raw: unsafe { default_mem() },
            is_faulty: false,
            delegate_lifetime: PhantomData,
        }
    }

    /// Create a faulty memory instance
    pub fn faulty(delegate: &'a Mem<'a>, e: &mut Error) -> Result<Self> {
        let raw = create(e, |d, ep| unsafe {
            faulty_mem_create(delegate.raw, 0.0, d, ep)
        })?;

        Ok(Self {
            raw,
            is_faulty: true,
            delegate_lifetime: PhantomData,
        })
    }

    pub fn malloc<S, L>(&self, size: S, len: L, e: &mut Error) -> Result<Block<'_>>
    where
        S: Into<NonZeroU32>,
        L: Into<NonZeroU32>,
    {
        let size = size.into();
        let len = len.into();

        // Call the c function
        let p = unsafe {
            ((*self.raw.table).malloc)(self.raw.self_, len.into(), size.into(), e.as_mut_ptr())
        };

        self.alloc_result(p, size.into(), len.into(), e)
    }

    pub fn calloc<S, L>(&self, size: S, len: L, e: &mut Error) -> Result<Block<'_>>
    where
        S: Into<NonZeroU32>,
        L: Into<NonZeroU32>,
    {
        let size = size.into();
        let len = len.into();

        // Call the c function
        let p = unsafe {
            ((*self.raw.table).calloc)(self.raw.self_, len.into(), size.into(), e.as_mut_ptr())
        };

        self.alloc_result(p, size, len, e)
    }

    fn alloc_result(
        &self,
        p: *mut c_void,
        size: NonZeroU32,
        len: NonZeroU32,
        e: &mut Error,
    ) -> Result<Block<'_>> {
        // Cast to a return value
        match NonNull::new(p.cast::<u8>()) {
            Some(ptr) => Ok(Block {
                ptr,
                len,
                size,
                mem: self,
            }),
            None => Err(e.err()),
        }
    }

    /// Faulty Setters
    pub fn faulty_set_percent(&self, fail_percent: f32) {
        assert!(self.is_faulty);
        unsafe {
            faulty_mem_set_fail(self.raw, fail_percent);
        }
    }

    pub fn faulty_fail_after(&self, count: c_int) {
        assert!(self.is_faulty);
        unsafe {
            faulty_mem_fail_after(self.raw, count);
        }
    }

    pub fn faulty_reset(&self) {
        assert!(self.is_faulty);
        unsafe {
            faulty_mem_reset(self.raw);
        }
    }

    pub fn faulty_mallocs(&self) -> u32 {
        assert!(self.is_faulty);
        unsafe { faulty_num_mallocs(self.raw) }
    }
}

impl<'a> Drop for Mem<'a> {
    fn drop(&mut self) {
        if self.is_faulty {
            unsafe { faulty_mem_free(self.raw) }
        }
    }
}

pub struct Block<'m> {
    pub ptr: NonNull<u8>,
    pub len: NonZeroU32,
    pub size: NonZeroU32,
    mem: &'m Mem<'m>,
}

impl<'a> Block<'a> {
    pub fn realloc<S, L>(&mut self, size: S, len: L, e: &mut Error) -> Result<()>
    where
        S: Into<NonZeroU32>,
        L: Into<NonZeroU32>,
    {
        let size = size.into();
        let len = len.into();

        let p = unsafe {
            ((*self.mem.raw.table).realloc)(
                self.mem.raw.self_,
                self.ptr.as_ptr().cast(),
                len.into(),
                size.into(),
                e.as_mut_ptr(),
            )
        };

        let ptr = NonNull::new(p.cast::<u8>()).ok_or_else(|| e.err())?;

        self.ptr = ptr;
        self.len = len;
        self.size = size;
        Ok(())
    }

    pub fn bytes(&self) -> &'a [u8] {
        let total = (self.len.get() as usize) * (self.size.get() as usize);
        unsafe { core::slice::from_raw_parts(self.ptr.as_ptr(), total) }
    }

    pub fn bytes_mut(&mut self) -> &'a mut [u8] {
        let total = (self.len.get() as usize) * (self.size.get() as usize);
        unsafe { core::slice::from_raw_parts_mut(self.ptr.as_ptr(), total) }
    }
}

impl Drop for Block<'_> {
    fn drop(&mut self) {
        unsafe {
            ((*self.mem.raw.table).free)(self.mem.raw.self_, self.ptr.as_ptr().cast());
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    // malloc

    #[test]
    fn system_malloc() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();

        let mut block = mem.malloc(nz!(4), nz!(16), &mut e)?;
        block.bytes_mut().fill(0xAB);
        assert_eq!(block.size.get(), 4);
        assert_eq!(block.len.get(), 16);
        assert!(block.bytes().iter().all(|&b| b == 0xAB));

        Ok(())
    }

    // calloc

    #[test]
    fn calloc_returns_zeroed_memory() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();

        let block = mem.calloc(nz!(8), nz!(32), &mut e)?;
        assert_eq!(block.size.get(), 8);
        assert_eq!(block.len.get(), 32);
        assert!(block.bytes().iter().all(|&b| b == 0));

        Ok(())
    }

    #[test]
    fn calloc_fails() -> Result<()> {
        let mut e = Error::new();
        let system = Mem::system();
        let mem = Mem::faulty(&system, &mut e)?;

        mem.faulty_fail_after(0);
        assert!(mem.calloc(nz!(8), nz!(8), &mut e).is_err());

        Ok(())
    }

    // realloc

    #[test]
    fn realloc_grow_preserves_data_and_updates_dims() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();

        let mut block = mem.malloc(nz!(4), nz!(4), &mut e)?;
        for (i, b) in block.bytes_mut().iter_mut().enumerate() {
            *b = i as u8;
        }

        block.realloc(nz!(4), nz!(8), &mut e)?;

        assert_eq!(block.size.get(), 4);
        assert_eq!(block.len.get(), 8);
        for (i, &b) in block.bytes()[..16].iter().enumerate() {
            assert_eq!(b, i as u8);
        }

        Ok(())
    }

    #[test]
    fn realloc_shrink_preserves_prefix() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();

        let mut block = mem.malloc(nz!(1), nz!(16), &mut e)?;
        for (i, b) in block.bytes_mut().iter_mut().enumerate() {
            *b = i as u8;
        }

        block.realloc(nz!(1), nz!(4), &mut e)?;

        assert_eq!(block.len.get(), 4);
        assert_eq!(block.bytes(), &[0, 1, 2, 3]);

        Ok(())
    }

    #[test]
    fn realloc_failure_returns_err_and_leaves_block_unchanged() -> Result<()> {
        let mut e = Error::new();
        let system = Mem::system();
        let mem = Mem::faulty(&system, &mut e)?;

        let mut block = mem.malloc(nz!(4), nz!(4), &mut e)?;
        block.bytes_mut().fill(0x7F);
        let old_ptr = block.ptr;

        // Bigger realloc
        mem.faulty_fail_after(0);
        assert!(block.realloc(nz!(4), nz!(1024), &mut e).is_err());
        assert_eq!(block.ptr, old_ptr);
        assert_eq!(block.len.get(), 4);
        assert_eq!(block.size.get(), 4);
        assert!(block.bytes().iter().all(|&b| b == 0x7F));
        e.reset();

        // Smaller realloc
        mem.faulty_fail_after(0);
        assert!(block.realloc(nz!(4), nz!(2), &mut e).is_err());
        assert_eq!(block.ptr, old_ptr);
        assert_eq!(block.len.get(), 4);
        assert_eq!(block.size.get(), 4);
        assert!(block.bytes().iter().all(|&b| b == 0x7F));

        Ok(())
    }

    // faulty

    #[test]
    fn faulty_default_zero_percent_never_fails() -> Result<()> {
        let mut e = Error::new();
        let system = Mem::system();
        let mem = Mem::faulty(&system, &mut e)?;
        for _ in 0..50 {
            let _b = mem.malloc(nz!(8), nz!(8), &mut e)?;
        }
        Ok(())
    }

    #[test]
    fn faulty_malloc_fails_when_fail_after_zero() -> Result<()> {
        let mut e = Error::new();
        let system = Mem::system();
        let mem = Mem::faulty(&system, &mut e)?;
        mem.faulty_fail_after(0);
        assert!(mem.malloc(nz!(10), nz!(100), &mut e).is_err());
        Ok(())
    }

    #[test]
    fn faulty_set_percent_one_always_fails() -> Result<()> {
        let mut e = Error::new();
        let system = Mem::system();
        let mem = Mem::faulty(&system, &mut e)?;
        mem.faulty_set_percent(1.0);
        for _ in 0..20 {
            assert!(mem.malloc(nz!(8), nz!(8), &mut e).is_err());
            e.reset();
        }
        Ok(())
    }

    #[test]
    fn faulty_set_percent_zero_never_fails() -> Result<()> {
        let mut e = Error::new();
        let system = Mem::system();
        let mem = Mem::faulty(&system, &mut e)?;
        mem.faulty_set_percent(1.0);
        mem.faulty_set_percent(0.0);
        for _ in 0..20 {
            let _b = mem.malloc(nz!(8), nz!(8), &mut e)?;
        }
        Ok(())
    }

    #[test]
    fn faulty_fail_after_n_allows_n_then_fails() -> Result<()> {
        let mut e = Error::new();
        let system = Mem::system();
        let mem = Mem::faulty(&system, &mut e)?;
        mem.faulty_fail_after(2);

        let _a = mem.malloc(nz!(8), nz!(1), &mut e)?;
        let _b = mem.malloc(nz!(8), nz!(1), &mut e)?;
        assert!(mem.malloc(nz!(8), nz!(1), &mut e).is_err());
        Ok(())
    }

    #[test]
    fn faulty_reset_clears_failure_mode() -> Result<()> {
        let mut e = Error::new();
        let system = Mem::system();
        let mem = Mem::faulty(&system, &mut e)?;

        mem.faulty_fail_after(0);
        assert!(mem.malloc(nz!(8), nz!(1), &mut e).is_err());
        e.reset();

        mem.faulty_reset();
        let _b = mem.malloc(nz!(8), nz!(1), &mut e)?;
        Ok(())
    }

    #[test]
    fn faulty_mallocs_starts_at_zero() -> Result<()> {
        let mut e = Error::new();
        let system = Mem::system();
        let mem = Mem::faulty(&system, &mut e)?;
        assert_eq!(mem.faulty_mallocs(), 0);
        Ok(())
    }

    #[test]
    fn faulty_mallocs_counts_allocations() -> Result<()> {
        let mut e = Error::new();
        let system = Mem::system();
        let mem = Mem::faulty(&system, &mut e)?;

        let _a = mem.malloc(nz!(8), nz!(1), &mut e)?;
        let _b = mem.malloc(nz!(8), nz!(1), &mut e)?;
        let _c = mem.malloc(nz!(8), nz!(1), &mut e)?;
        assert_eq!(mem.faulty_mallocs(), 3);
        Ok(())
    }

    #[test]
    fn faulty_reset_zeroes_malloc_count() -> Result<()> {
        let mut e = Error::new();
        let system = Mem::system();
        let mem = Mem::faulty(&system, &mut e)?;
        let _a = mem.malloc(nz!(8), nz!(1), &mut e)?;
        mem.faulty_reset();
        assert_eq!(mem.faulty_mallocs(), 0);
        Ok(())
    }

    // non-faulty guard assertions

    #[test]
    #[should_panic]
    fn faulty_set_percent_panics_on_system_mem() {
        Mem::system().faulty_set_percent(0.5);
    }

    #[test]
    #[should_panic]
    fn faulty_fail_after_panics_on_system_mem() {
        Mem::system().faulty_fail_after(1);
    }

    #[test]
    #[should_panic]
    fn faulty_reset_panics_on_system_mem() {
        Mem::system().faulty_reset();
    }

    #[test]
    #[should_panic]
    fn faulty_mallocs_panics_on_system_mem() {
        let _ = Mem::system().faulty_mallocs();
    }

    // Drop

    #[test]
    fn block_drop_and_faulty_drop_do_not_crash() -> Result<()> {
        let mut e = Error::new();
        let system = Mem::system();
        {
            let mem = Mem::faulty(&system, &mut e)?;
            let a = mem.malloc(nz!(8), nz!(8), &mut e)?;
            let b = mem.calloc(nz!(8), nz!(8), &mut e)?;
            drop(a);
            drop(b);
        } // faulty Mem freed here, after its blocks
        Ok(())
    }
}
