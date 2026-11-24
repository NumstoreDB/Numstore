#![allow(non_camel_case_types)]

use core::ffi::{c_char, c_int};

pub type err_t = c_int;

#[repr(C)]
pub struct error {
    pub cause_code: err_t,
    pub cause_msg: [c_char; 256],
    pub cmlen: u32,
    pub disable_log: bool,
}

const _: () = assert!(core::mem::size_of::<error>() == 268);
const _: () = assert!(core::mem::offset_of!(error, cmlen) == 260);
