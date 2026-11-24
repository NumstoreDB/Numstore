#![allow(non_camel_case_types)]

pub type f16 = u16;

pub type cf32 = [u16; 2];
pub type cf64 = [u32; 2];
pub type cf128 = [u64; 2];

pub type ci16 = [i8; 2];
pub type ci32 = [i16; 2];
pub type ci64 = [i32; 2];
pub type ci128 = [i64; 2];

pub type cu16 = [u8; 2];
pub type cu32 = [u16; 2];
pub type cu64 = [u32; 2];
pub type cu128 = [u64; 2];

pub const NS_PAGE_SIZE: usize = 4096;
pub const MEMORY_PAGE_LEN: usize = 4096;
pub const WAL_BUFFER_CAP: usize = 1_048_576;
pub const MAX_ERRSTR_LEN: usize = 256;

pub const NS_END: i64 = i64::MAX;

pub type t_size = u32;
pub type st_size = i32;
pub type p_size = u32;
pub type sp_size = i32;
pub type b_size = u64;
pub type sb_size = i64;
pub type pgno = u64;
pub type spgno = i64;
pub type txid = u64;
pub type stxid = i64;
pub type slsn = i64;
pub type lsn = u64;
pub type pgh = u8;
pub type wlh = u8;

pub const PGNO_NULL: pgno = U64_MAX;
pub const LSN_NULL: lsn = U64_MAX;
pub const WLH_NULL: wlh = U8_MAX;

pub const MAX_OPEN_FILES: usize = 64;
pub const MAX_THREADS: usize = 128;
const _: () = assert!(MAX_OPEN_FILES.is_power_of_two());
const _: () = assert!(MAX_THREADS.is_power_of_two());

pub const U8_MAX: u8 = u8::MAX;
pub const U16_MAX: u16 = u16::MAX;
pub const U32_MAX: u32 = u32::MAX;
pub const U64_MAX: u64 = u64::MAX;

pub const I8_MAX: i8 = i8::MAX;
pub const I16_MAX: i16 = i16::MAX;
pub const I32_MAX: i32 = i32::MAX;
pub const I64_MAX: i64 = i64::MAX;

pub const I8_ABS_MAX: u8 = I8_MAX as u8 + 1;
pub const I16_ABS_MAX: u16 = I16_MAX as u16 + 1;
pub const I32_ABS_MAX: u32 = I32_MAX as u32 + 1;
pub const I64_ABS_MAX: u64 = I64_MAX as u64 + 1;

pub const I8_MIN: i8 = i8::MIN;
pub const I16_MIN: i16 = i16::MIN;
pub const I32_MIN: i32 = i32::MIN;
pub const I64_MIN: i64 = i64::MIN;

pub const F16_MAX: f32 = 65504.0;
pub const F32_MAX: f32 = f32::MAX;
pub const F64_MAX: f64 = f64::MAX;
pub const F16_MIN: f32 = -65504.0;
pub const F32_MIN: f32 = -f32::MAX;
pub const F64_MIN: f64 = -f64::MAX;
