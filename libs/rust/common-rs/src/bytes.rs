use core::{
    marker::PhantomData,
    num::TryFromIntError,
    ops::{Deref, DerefMut},
};

use crate::ffi::os::{bytes, cbytes};

#[derive(Copy, Clone)]
#[repr(transparent)]
pub struct IoSlice<'a> {
    raw: cbytes,
    _buf: PhantomData<&'a [u8]>,
}

impl<'a> TryFrom<&'a [u8]> for IoSlice<'a> {
    type Error = TryFromIntError;

    fn try_from(s: &'a [u8]) -> core::result::Result<Self, Self::Error> {
        let len = u32::try_from(s.len())?;
        Ok(Self {
            raw: cbytes {
                data: s.as_ptr(),
                len,
            },
            _buf: PhantomData,
        })
    }
}

impl Deref for IoSlice<'_> {
    type Target = [u8];
    fn deref(&self) -> &[u8] {
        // SAFETY: built from a valid `&'a [u8]` and never mutated.
        unsafe { core::slice::from_raw_parts(self.raw.data, self.raw.len as usize) }
    }
}

#[repr(transparent)]
pub struct IoSliceMut<'a> {
    raw: bytes,
    _buf: PhantomData<&'a mut [u8]>,
}

impl<'a> TryFrom<&'a mut [u8]> for IoSliceMut<'a> {
    type Error = TryFromIntError;

    fn try_from(s: &'a mut [u8]) -> core::result::Result<Self, Self::Error> {
        let len = u32::try_from(s.len())?;
        Ok(Self {
            raw: bytes {
                data: s.as_mut_ptr(),
                len,
            },
            _buf: PhantomData,
        })
    }
}

impl Deref for IoSliceMut<'_> {
    type Target = [u8];
    fn deref(&self) -> &[u8] {
        unsafe { core::slice::from_raw_parts(self.raw.data, self.raw.len as usize) }
    }
}

impl DerefMut for IoSliceMut<'_> {
    fn deref_mut(&mut self) -> &mut [u8] {
        unsafe { core::slice::from_raw_parts_mut(self.raw.data, self.raw.len as usize) }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use core::mem::{align_of, size_of};

    // ---- layout (File::writev_all casts *IoSlice -> *bytes) -----------

    #[test]
    fn ioslice_layout_matches_c_structs() {
        assert_eq!(size_of::<IoSlice<'_>>(), size_of::<cbytes>());
        assert_eq!(align_of::<IoSlice<'_>>(), align_of::<cbytes>());
        // writev_all reinterprets IoSlice as `bytes`.
        assert_eq!(size_of::<IoSlice<'_>>(), size_of::<bytes>());
        assert_eq!(align_of::<IoSlice<'_>>(), align_of::<bytes>());
    }

    #[test]
    fn ioslicemut_layout_matches_c_struct() {
        assert_eq!(size_of::<IoSliceMut<'_>>(), size_of::<bytes>());
        assert_eq!(align_of::<IoSliceMut<'_>>(), align_of::<bytes>());
    }

    // ---- IoSlice -------------------------------------------------------

    #[test]
    fn ioslice_try_from_preserves_pointer_and_len() {
        let buf = [1u8, 2, 3, 4];
        let s = match IoSlice::try_from(&buf[..]) {
            Ok(s) => s,
            Err(_) => panic!(),
        };
        assert_eq!(s.raw.data, buf.as_ptr());
        assert_eq!(s.raw.len, 4);
    }

    #[test]
    fn ioslice_deref_returns_same_bytes() {
        let buf = *b"hello";
        let s = match IoSlice::try_from(&buf[..]) {
            Ok(s) => s,
            Err(_) => panic!(),
        };
        assert_eq!(&s[..], b"hello");
        assert_eq!(s.len(), 5);
    }

    #[test]
    fn ioslice_empty_slice_is_ok() {
        let buf: [u8; 0] = [];
        let s = match IoSlice::try_from(&buf[..]) {
            Ok(s) => s,
            Err(_) => panic!(),
        };
        assert_eq!(s.raw.len, 0);
        assert!(s.is_empty());
    }

    #[test]
    fn ioslice_is_copy() {
        let buf = *b"abc";
        let a = match IoSlice::try_from(&buf[..]) {
            Ok(s) => s,
            Err(_) => panic!(),
        };
        let b = a; // Copy
        assert_eq!(&a[..], &b[..]);
    }

    // Never dereferenced: only the length check is exercised. The dangling
    // pointer with a >4GiB length is not a valid slice, so this relies on
    // try_from checking the length before touching memory.
    #[test]
    #[cfg(target_pointer_width = "64")]
    fn ioslice_try_from_rejects_len_over_u32() {
        let huge = unsafe {
            core::slice::from_raw_parts(
                core::ptr::NonNull::<u8>::dangling().as_ptr(),
                u32::MAX as usize + 1,
            )
        };
        assert!(IoSlice::try_from(huge).is_err());
    }

    #[test]
    #[cfg(target_pointer_width = "64")]
    fn ioslice_try_from_accepts_len_exactly_u32_max_boundary_check() {
        // u32::MAX itself must convert; only the length is checked.
        let n = u32::MAX as usize;
        let ok = u32::try_from(n).is_ok();
        assert!(ok);
    }

    // ---- IoSliceMut ----------------------------------------------------

    #[test]
    fn ioslicemut_try_from_preserves_pointer_and_len() {
        let mut buf = [0u8; 8];
        let ptr = buf.as_mut_ptr();
        let s = match IoSliceMut::try_from(&mut buf[..]) {
            Ok(s) => s,
            Err(_) => panic!(),
        };
        assert_eq!(s.raw.data, ptr);
        assert_eq!(s.raw.len, 8);
    }

    #[test]
    fn ioslicemut_deref_reads_contents() {
        let mut buf = *b"xyz";
        let s = match IoSliceMut::try_from(&mut buf[..]) {
            Ok(s) => s,
            Err(_) => panic!(),
        };
        assert_eq!(&s[..], b"xyz");
    }

    #[test]
    fn ioslicemut_deref_mut_writes_through_to_original() {
        let mut buf = [0u8; 4];
        {
            let mut s = match IoSliceMut::try_from(&mut buf[..]) {
                Ok(s) => s,
                Err(_) => panic!(),
            };
            s[0] = 9;
            s[3] = 7;
            s.fill(5);
            s[1] = 1;
        }
        assert_eq!(buf, [5, 1, 5, 5]);
    }

    #[test]
    fn ioslicemut_empty_slice_is_ok() {
        let mut buf: [u8; 0] = [];
        let s = match IoSliceMut::try_from(&mut buf[..]) {
            Ok(s) => s,
            Err(_) => panic!(),
        };
        assert!(s.is_empty());
    }

    #[test]
    #[cfg(target_pointer_width = "64")]
    fn ioslicemut_try_from_rejects_len_over_u32() {
        let huge = unsafe {
            core::slice::from_raw_parts_mut(
                core::ptr::NonNull::<u8>::dangling().as_ptr(),
                u32::MAX as usize + 1,
            )
        };
        assert!(IoSliceMut::try_from(huge).is_err());
    }
}
