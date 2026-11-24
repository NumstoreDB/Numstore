#![allow(non_camel_case_types)]

use core::ffi::c_int;
use core::mem::ManuallyDrop;

use crate::bytes::IoSlice;
use crate::error::{Error, Result};
use crate::ffi::error::error;
use crate::ffi::os::{bytes, file_pread_all_expect, file_read_all_expect, i_file, seek_t};
use crate::os::Os;

pub struct File<'os> {
    pub raw: i_file,
    pub os: &'os Os<'os>,
}

impl<'os> File<'os> {
    pub fn new(raw: i_file, os: &'os Os<'os>) -> Self {
        Self { raw, os }
    }
}

impl File<'_> {
    pub fn fsync(&self, e: &mut Error) -> Result<()> {
        if unsafe { (self.raw.vt().fsync)(self.raw.self_, e.as_mut_ptr()) } < 0 {
            Err(e.err())
        } else {
            Ok(())
        }
    }

    pub fn size(&self, e: &mut Error) -> Result<u64> {
        let ret = unsafe { (self.raw.vt().file_size)(self.raw.self_, e.as_mut_ptr()) };

        if ret < 0 {
            Err(e.err())
        } else {
            Ok(ret as u64)
        }
    }

    /// Read up to `buf.len()` bytes (fewer at EOF); returns the count.
    pub fn read_all(&self, buf: &mut [u8], e: &mut Error) -> Result<usize> {
        let ret = unsafe {
            (self.raw.vt().read_all)(
                self.raw.self_,
                buf.as_mut_ptr().cast(),
                buf.len() as u64,
                e.as_mut_ptr(),
            )
        };

        if ret < 0 {
            Err(e.err())
        } else {
            Ok(ret as usize)
        }
    }

    /// Like [`File::read_all`] at `offset`, without moving the file position.
    pub fn pread_all(&self, buf: &mut [u8], offset: u64, e: &mut Error) -> Result<usize> {
        let ret = unsafe {
            (self.raw.vt().pread_all)(
                self.raw.self_,
                buf.as_mut_ptr().cast(),
                buf.len() as u64,
                offset,
                e.as_mut_ptr(),
            )
        };

        if ret < 0 {
            Err(e.err())
        } else {
            Ok(ret as usize)
        }
    }

    /// Fill `buf` completely or fail (the C `file_read_all_expect`).
    pub fn read_exact(&self, buf: &mut [u8], e: &mut Error) -> Result<()> {
        let ret = unsafe {
            file_read_all_expect(
                self.raw,
                buf.as_mut_ptr().cast(),
                buf.len() as u64,
                e.as_mut_ptr(),
            )
        };

        if ret < 0 {
            Err(e.err())
        } else {
            Ok(())
        }
    }

    /// Fill `buf` from `offset` or fail (the C `file_pread_all_expect`).
    pub fn pread_exact(&self, buf: &mut [u8], offset: u64, e: &mut Error) -> Result<()> {
        let ret = unsafe {
            file_pread_all_expect(
                self.raw,
                buf.as_mut_ptr().cast(),
                buf.len() as u64,
                offset,
                e.as_mut_ptr(),
            )
        };

        if ret < 0 {
            Err(e.err())
        } else {
            Ok(())
        }
    }

    pub fn write_all(&self, buf: &[u8], e: &mut Error) -> Result<()> {
        let ret = unsafe {
            (self.raw.vt().write_all)(
                self.raw.self_,
                buf.as_ptr().cast(),
                buf.len() as u64,
                e.as_mut_ptr(),
            )
        };

        if ret < 0 {
            Err(e.err())
        } else {
            Ok(())
        }
    }

    pub fn pwrite_all(&self, buf: &[u8], offset: u64, e: &mut Error) -> Result<()> {
        let ret = unsafe {
            (self.raw.vt().pwrite_all)(
                self.raw.self_,
                buf.as_ptr().cast(),
                buf.len() as u64,
                offset,
                e.as_mut_ptr(),
            )
        };

        if ret < 0 {
            Err(e.err())
        } else {
            Ok(())
        }
    }

    /// Write every slice in order.
    ///
    /// # Panics
    /// If `bufs.len()` exceeds `c_int::MAX`.
    pub fn writev_all(&self, bufs: &[IoSlice<'_>], e: &mut Error) -> Result<()> {
        let n = c_int::try_from(bufs.len()).expect("too many buffers");
        let ptr = bufs.as_ptr().cast::<bytes>().cast_mut();
        let ret = unsafe { (self.raw.vt().writev_all)(self.raw.self_, ptr, n, e.as_mut_ptr()) };

        if ret < 0 {
            Err(e.err())
        } else {
            Ok(())
        }
    }

    pub fn truncate(&self, len: u64, e: &mut Error) -> Result<()> {
        let ret = unsafe { (self.raw.vt().truncate)(self.raw.self_, len, e.as_mut_ptr()) };

        if ret < 0 {
            Err(e.err())
        } else {
            Ok(())
        }
    }

    pub fn prealloc(&self, len: u64, e: &mut Error) -> Result<()> {
        let ret = unsafe { (self.raw.vt().prealloc)(self.raw.self_, len, e.as_mut_ptr()) };

        if ret < 0 {
            Err(e.err())
        } else {
            Ok(())
        }
    }

    /// Seek; returns the new position.
    pub fn seek(&self, offset: u64, whence: seek_t, e: &mut Error) -> Result<u64> {
        let ret =
            unsafe { (self.raw.vt().seek)(self.raw.self_, offset, whence.into(), e.as_mut_ptr()) };

        if ret < 0 {
            Err(e.err())
        } else {
            Ok(ret as u64)
        }
    }

    pub fn close(self, e: &mut Error) -> Result<()> {
        let this = ManuallyDrop::new(self); // skip Drop: we close here
        let ret = unsafe {
            (this.os.raw.vt().close_file)(this.os.raw.self_(), this.raw.self_, e.as_mut_ptr())
        };

        if ret < 0 {
            Err(e.err())
        } else {
            Ok(())
        }
    }
}

impl Drop for File<'_> {
    fn drop(&mut self) {
        // No caller-provided `Error` here; all-zero is the clean state.
        let mut e: error = unsafe { core::mem::zeroed() };
        unsafe { (self.os.raw.vt().close_file)(self.os.raw.self_(), self.raw.self_, &mut e) };
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::memory::Mem;

    fn io(s: &[u8]) -> IoSlice<'_> {
        match IoSlice::try_from(s) {
            Ok(v) => v,
            Err(_) => panic!("slice too large"),
        }
    }

    struct TmpFile<'a> {
        os: &'a Os<'a>,
        path: &'static core::ffi::CStr,
    }

    impl<'a> TmpFile<'a> {
        fn new(os: &'a Os<'a>, path: &'static core::ffi::CStr) -> Result<Self> {
            let mut e = Error::new();
            os.remove_quiet(path, &mut e)?;
            Ok(Self { os, path })
        }
    }

    impl Drop for TmpFile<'_> {
        fn drop(&mut self) {
            let mut e = Error::new();
            let _ = self.os.remove_quiet(self.path, &mut e);
        }
    }

    /// Create `path` containing `data`.
    fn seeded(os: &Os<'_>, path: &core::ffi::CStr, data: &[u8], e: &mut Error) -> Result<()> {
        let f = os.open_w(path, e)?;
        if data.len() > 0 {
            f.write_all(data, e)?;
        }
        f.close(e)
    }

    // write_all / size / fsync

    #[test]
    fn new_file_has_size_zero() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_size_zero.tmp";
        let _g = TmpFile::new(&os, path)?;

        let f = os.open_w(path, &mut e)?;
        assert_eq!(f.size(&mut e)?, 0);
        Ok(())
    }

    #[test]
    fn write_all_grows_size() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_write_size.tmp";
        let _g = TmpFile::new(&os, path)?;

        let f = os.open_w(path, &mut e)?;
        f.write_all(b"hello", &mut e)?;
        assert_eq!(f.size(&mut e)?, 5);
        f.write_all(b" world", &mut e)?;
        assert_eq!(f.size(&mut e)?, 11);
        Ok(())
    }

    #[test]
    fn fsync_succeeds_on_open_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_fsync.tmp";
        let _g = TmpFile::new(&os, path)?;

        let f = os.open_w(path, &mut e)?;
        f.write_all(b"data", &mut e)?;
        f.fsync(&mut e)?;
        Ok(())
    }

    // read_all

    #[test]
    fn read_all_returns_written_bytes() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_read_all.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"abcdef", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        let mut buf = [0u8; 6];
        assert_eq!(f.read_all(&mut buf, &mut e)?, 6);
        assert_eq!(&buf, b"abcdef");
        Ok(())
    }

    #[test]
    fn read_all_short_at_eof() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_read_all_short.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"abcd", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        let mut buf = [0u8; 16];
        assert_eq!(f.read_all(&mut buf, &mut e)?, 4);
        assert_eq!(&buf[..4], b"abcd");
        Ok(())
    }

    #[test]
    fn read_all_on_empty_file_returns_zero() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_read_all_empty.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        let mut buf = [0u8; 8];
        assert_eq!(f.read_all(&mut buf, &mut e)?, 0);
        Ok(())
    }

    #[test]
    fn read_all_advances_position() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_read_advances.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"abcdef", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        let mut a = [0u8; 3];
        let mut b = [0u8; 3];
        f.read_all(&mut a, &mut e)?;
        f.read_all(&mut b, &mut e)?;
        assert_eq!(&a, b"abc");
        assert_eq!(&b, b"def");
        Ok(())
    }

    #[test]
    fn read_all_fails_on_write_only_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_read_wronly.tmp";
        let _g = TmpFile::new(&os, path)?;

        let f = os.open_w(path, &mut e)?;
        let mut buf = [0u8; 4];
        assert!(f.read_all(&mut buf, &mut e).is_err());
        Ok(())
    }

    // pread_all

    #[test]
    fn pread_all_reads_at_offset() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_pread.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"0123456789", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        let mut buf = [0u8; 4];
        assert_eq!(f.pread_all(&mut buf, 3, &mut e)?, 4);
        assert_eq!(&buf, b"3456");
        Ok(())
    }

    #[test]
    fn pread_all_does_not_move_position() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_pread_pos.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"0123456789", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        let mut tail = [0u8; 2];
        f.pread_all(&mut tail, 8, &mut e)?;

        let mut head = [0u8; 2];
        f.read_all(&mut head, &mut e)?;
        assert_eq!(&head, b"01");
        Ok(())
    }

    #[test]
    fn pread_all_short_near_eof_and_zero_past_eof() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_pread_eof.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"abcd", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        let mut buf = [0u8; 8];
        assert_eq!(f.pread_all(&mut buf, 2, &mut e)?, 2);
        assert_eq!(f.pread_all(&mut buf, 100, &mut e)?, 0);
        Ok(())
    }

    // read_exact / pread_exact

    #[test]
    fn read_exact_fills_buffer() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_read_exact.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"abcdef", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        let mut buf = [0u8; 6];
        f.read_exact(&mut buf, &mut e)?;
        assert_eq!(&buf, b"abcdef");
        Ok(())
    }

    #[test]
    fn read_exact_fails_when_file_too_short() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_read_exact_short.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"abc", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        let mut buf = [0u8; 8];
        assert!(f.read_exact(&mut buf, &mut e).is_err());
        Ok(())
    }

    #[test]
    fn pread_exact_fills_buffer_at_offset() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_pread_exact.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"0123456789", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        let mut buf = [0u8; 3];
        f.pread_exact(&mut buf, 5, &mut e)?;
        assert_eq!(&buf, b"567");
        Ok(())
    }

    #[test]
    fn pread_exact_fails_past_eof() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_pread_exact_eof.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"abcd", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        let mut buf = [0u8; 4];
        assert!(f.pread_exact(&mut buf, 2, &mut e).is_err());
        Ok(())
    }

    // ---- write_all failures / pwrite_all ------------------------------

    #[test]
    fn write_all_fails_on_read_only_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_write_rdonly.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"x", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        assert!(f.write_all(b"nope", &mut e).is_err());
        Ok(())
    }

    #[test]
    fn pwrite_all_writes_at_offset() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_pwrite.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"AAAAAAAA", &mut e)?;

        let f = os.open_rw(path, &mut e)?;
        f.pwrite_all(b"xy", 3, &mut e)?;

        let mut buf = [0u8; 8];
        f.pread_exact(&mut buf, 0, &mut e)?;
        assert_eq!(&buf, b"AAAxyAAA");
        Ok(())
    }

    #[test]
    fn pwrite_all_past_end_extends_with_zeros() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_pwrite_hole.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"", &mut e)?;

        let f = os.open_rw(path, &mut e)?;
        f.pwrite_all(b"WXYZ", 4, &mut e)?;
        assert_eq!(f.size(&mut e)?, 8);

        let mut buf = [0xFFu8; 8];
        f.pread_exact(&mut buf, 0, &mut e)?;
        assert_eq!(&buf, b"\0\0\0\0WXYZ");
        Ok(())
    }

    // ---- writev_all ----------------------------------------------------

    #[test]
    fn writev_all_writes_slices_in_order() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_writev.tmp";
        let _g = TmpFile::new(&os, path)?;

        let f = os
            .open_rw(path, &mut e)
            .or_else(|_| os.open_w(path, &mut e))?;

        let bufs = [io(b"foo"), io(b"-")];
        f.writev_all(&bufs, &mut e)?;
        assert_eq!(f.size(&mut e)?, 4);

        let mut out = [0u8; 4];
        f.pread_exact(&mut out, 0, &mut e)?;
        assert_eq!(&out, b"foo-");
        Ok(())
    }

    // ---- truncate / prealloc ------------------------------------------

    #[test]
    fn truncate_shrinks_file() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_truncate_shrink.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"0123456789", &mut e)?;

        let f = os.open_rw(path, &mut e)?;
        f.truncate(4, &mut e)?;
        assert_eq!(f.size(&mut e)?, 4);

        let mut buf = [0u8; 4];
        f.pread_exact(&mut buf, 0, &mut e)?;
        assert_eq!(&buf, b"0123");
        Ok(())
    }

    #[test]
    fn truncate_grows_file_with_zeros() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_truncate_grow.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"ab", &mut e)?;

        let f = os.open_rw(path, &mut e)?;
        f.truncate(6, &mut e)?;
        assert_eq!(f.size(&mut e)?, 6);

        let mut buf = [0xFFu8; 6];
        f.pread_exact(&mut buf, 0, &mut e)?;
        assert_eq!(&buf, b"ab\0\0\0\0");
        Ok(())
    }

    #[test]
    fn prealloc_succeeds_and_does_not_shrink() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_prealloc.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"abcd", &mut e)?;

        let f = os.open_rw(path, &mut e)?;
        f.prealloc(4096, &mut e)?;
        assert!(f.size(&mut e)? >= 4);
        Ok(())
    }

    // ---- seek ----------------------------------------------------------

    #[test]
    fn seek_set_returns_new_position_and_affects_reads() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_seek_set.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"0123456789", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        assert_eq!(f.seek(5, seek_t::I_SEEK_SET, &mut e)?, 5);

        let mut buf = [0u8; 2];
        f.read_exact(&mut buf, &mut e)?;
        assert_eq!(&buf, b"56");
        Ok(())
    }

    #[test]
    fn seek_cur_is_relative() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_seek_cur.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"0123456789", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        f.seek(3, seek_t::I_SEEK_SET, &mut e)?;
        assert_eq!(f.seek(2, seek_t::I_SEEK_CUR, &mut e)?, 5);
        Ok(())
    }

    #[test]
    fn seek_end_zero_returns_file_size() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_seek_end.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"0123456789", &mut e)?;

        let f = os.open_r(path, &mut e)?;
        assert_eq!(f.seek(0, seek_t::I_SEEK_END, &mut e)?, 10);
        Ok(())
    }

    // ---- close / Drop --------------------------------------------------

    #[test]
    fn close_succeeds_and_file_persists() -> Result<()> {
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_close.tmp";
        let _g = TmpFile::new(&os, path)?;

        let f = os.open_w(path, &mut e)?;
        f.write_all(b"kept", &mut e)?;
        f.close(&mut e)?;

        assert!(os.file_exists(path, &mut e)?);
        let r = os.open_r(path, &mut e)?;
        assert_eq!(r.size(&mut e)?, 4);
        Ok(())
    }

    #[test]
    fn drop_closes_file_descriptor() -> Result<()> {
        // Exceeds the default fd limit (256 on macOS) if Drop leaked descriptors.
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_drop_fds.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"x", &mut e)?;

        for _ in 0..600 {
            let _f = os.open_r(path, &mut e)?;
        }
        Ok(())
    }

    #[test]
    fn close_does_not_double_close() -> Result<()> {
        // close() uses ManuallyDrop; many close() calls must not exhaust or
        // corrupt descriptors (a double close could hit a reused fd).
        let mut e = Error::new();
        let mem = Mem::system();
        let os = Os::system(&mem, &mut e)?;
        let path = c"/tmp/file_test_close_twice.tmp";
        let _g = TmpFile::new(&os, path)?;
        seeded(&os, path, b"x", &mut e)?;

        let keep = os.open_r(path, &mut e)?;
        for _ in 0..50 {
            os.open_r(path, &mut e)?.close(&mut e)?;
        }
        let mut buf = [0u8; 1];
        keep.read_exact(&mut buf, &mut e)?;
        assert_eq!(&buf, b"x");
        Ok(())
    }
}
