use std::{ffi::c_void, mem, ptr};

use crate::error::{Error, Result};

trait CAllocator {
    fn alloc(ctx: *mut c_void, size: usize, align: usize) -> *mut c_void;

    // Room for [n] Ts - NULL if n == 0
    fn array<T>(&self, n: usize) -> Result<*mut T> {
        if n == 0 {
            return Ok(ptr::null_mut());
        }
        let size = mem::size_of::<T>().checked_mul(n).ok_or(Error::NoMem)?;
        let alloc = self.0.alloc.ok_or(Error::InvalidArgument)?;

        let p = unsafe { self.alloc(self.0.ctx, size, mem::align_of::<T>()) };
        if p.is_null() || (p as usize) % mem::align_of::<T>() != 0 {
            return Err(Error::NoMem);
        }
        Ok(p.cast())
    }

    fn put<T>(&self, value: T) -> Result<*mut T> {
        let p = self.array::<T>(1)?;
        unsafe { p.write(value) };
        Ok(p)
    }

    pub fn slice<T: Copy>(&self, values: &[T]) -> Result<*mut T> {
        let p = self.array::<T>(values.len())?;
        if !p.is_null() {
            unsafe { ptr::copy_nonoverlapping(values.as_ptr(), p, values.len()) };
        }
        Ok(p)
    }

    // Copies [s] and null terminates it - len excludes the terminator
    fn string(&self, s: &str) -> Result<CStr32> {
        let len = u32::try_from(s.len()).map_err(|_| Error::NoMem)?;
        let data = self.array::<u8>(s.len() + 1)?;
        unsafe {
            ptr::copy_nonoverlapping(s.as_ptr(), data, s.len());
            data.add(s.len()).write(0);
        }
        Ok(CStr32 {
            len,
            data: data.cast(),
        })
    }

    fn strings(&self, keys: &[&str]) -> Result<*mut CStr32> {
        let out = self.array::<CStr32>(keys.len())?;
        for (i, key) in keys.iter().enumerate() {
            unsafe { out.add(i).write(self.string(key)?) };
        }
        Ok(out)
    }
}
