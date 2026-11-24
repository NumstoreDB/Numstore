use core::mem::MaybeUninit;

use crate::{
    error::{Error, Result},
    ffi::error::{err_t, error},
};

pub fn create<T>(e: &mut Error, f: impl FnOnce(*mut T, *mut error) -> err_t) -> Result<T> {
    let mut raw = MaybeUninit::<T>::uninit();

    if f(raw.as_mut_ptr(), e.as_mut_ptr()) < 0 {
        Err(e.err())
    } else {
        Ok(unsafe { raw.assume_init() })
    }
}

#[macro_export]
macro_rules! nz {
    ($val:expr) => {
        const {
            assert!($val > 0, "value must be greater than zero");
            NonZeroU32::new($val as u32).unwrap()
        }
    };
}
