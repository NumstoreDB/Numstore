#![allow(non_camel_case_types)]

use core::ffi::{c_char, c_int, c_void};

use crate::ffi::{
    error::{err_t, error},
    memory::i_mem,
};

/// Bytes
#[repr(C)]
#[derive(Copy, Clone)]
pub struct bytes {
    pub data: *mut u8,
    pub len: u32,
}

#[repr(C)]
#[derive(Copy, Clone)]
pub struct cbytes {
    pub data: *const u8,
    pub len: u32,
}

/// Condition Variables
#[repr(C)]
#[derive(Copy, Clone)]
pub struct i_cond {
    pub data: *const os_vtable,
    pub len: *mut c_void,
}

/// Mutex
#[repr(C)]
#[derive(Copy, Clone)]
pub struct i_mutex {
    pub data: *const os_vtable,
    pub len: *mut c_void,
}

/// Thread
#[repr(C)]
#[derive(Copy, Clone)]
pub struct i_thread {
    pub data: *const os_vtable,
    pub len: *mut c_void,
}

/// Operating System
#[repr(C)]
#[derive(Copy, Clone)]
pub struct i_os {
    pub vt: *const os_vtable,
    pub self_: *mut c_void,
}

impl i_os {
    pub fn vt(&self) -> &os_vtable {
        unsafe { &*self.vt }
    }

    pub fn self_(&self) -> *mut c_void {
        self.self_
    }
}

/// File
#[repr(C)]
#[derive(Copy, Clone)]
pub struct i_file {
    pub vt: *const os_vtable,
    pub self_: *mut c_void,
}

impl i_file {
    pub fn vt(&self) -> &os_vtable {
        unsafe { &*self.vt }
    }
}

/// Timer
#[repr(C)]
#[derive(Copy, Clone)]
pub struct i_timer {
    pub data: *const os_vtable,
    pub len: *mut c_void,
}

#[repr(C)]
#[derive(Copy, Clone, Debug, PartialEq, Eq)]
pub enum seek_t {
    I_SEEK_END = 0,
    I_SEEK_CUR = 1,
    I_SEEK_SET = 2,
}

#[repr(C)]
pub struct os_vtable {
    // Opening and closing files
    pub open_file_rw: unsafe extern "C" fn(
        os_self: *mut c_void,
        dest: *mut i_file,
        fname: *const c_char,
        e: *mut error,
    ) -> err_t,

    pub open_file_r: unsafe extern "C" fn(
        os_self: *mut c_void,
        dest: *mut i_file,
        fname: *const c_char,
        e: *mut error,
    ) -> err_t,

    pub open_file_w: unsafe extern "C" fn(
        os_self: *mut c_void,
        dest: *mut i_file,
        fname: *const c_char,
        e: *mut error,
    ) -> err_t,

    pub close_file:
        unsafe extern "C" fn(os_self: *mut c_void, file_self: *mut c_void, e: *mut error) -> err_t,

    // File system
    pub remove_quiet:
        unsafe extern "C" fn(os_self: *mut c_void, fname: *const c_char, e: *mut error) -> err_t,

    pub unlink:
        unsafe extern "C" fn(os_self: *mut c_void, fname: *const c_char, e: *mut error) -> err_t,

    pub file_exists: unsafe extern "C" fn(
        os_self: *mut c_void,
        fname: *const c_char,
        dest: *mut bool,
        e: *mut error,
    ) -> err_t,

    // File operations
    pub fsync: unsafe extern "C" fn(file_self: *mut c_void, e: *mut error) -> err_t,

    pub file_size: unsafe extern "C" fn(file_self: *mut c_void, e: *mut error) -> i64,

    pub read_all: unsafe extern "C" fn(
        file_self: *mut c_void,
        dest: *mut c_void,
        nbytes: u64,
        e: *mut error,
    ) -> i64,

    pub pread_all: unsafe extern "C" fn(
        file_self: *mut c_void,
        dest: *mut c_void,
        n: u64,
        offset: u64,
        e: *mut error,
    ) -> i64,

    pub write_all: unsafe extern "C" fn(
        file_self: *mut c_void,
        src: *const c_void,
        nbytes: u64,
        e: *mut error,
    ) -> err_t,

    pub pwrite_all: unsafe extern "C" fn(
        file_self: *mut c_void,
        src: *const c_void,
        n: u64,
        offset: u64,
        e: *mut error,
    ) -> err_t,

    pub writev_all: unsafe extern "C" fn(
        file_self: *mut c_void,
        arrs: *mut bytes,
        iovcnt: c_int,
        e: *mut error,
    ) -> err_t,

    pub truncate: unsafe extern "C" fn(file_self: *mut c_void, bytes: u64, e: *mut error) -> err_t,

    pub prealloc: unsafe extern "C" fn(file_self: *mut c_void, bytes: u64, e: *mut error) -> err_t,

    pub seek: unsafe extern "C" fn(
        file_self: *mut c_void,
        offset: u64,
        whence: seek_t,
        e: *mut error,
    ) -> i64,

    // Threads
    pub thread_create: unsafe extern "C" fn(
        os_self: *mut c_void,
        dest: *mut i_thread,
        start: unsafe extern "C" fn(&mut c_void) -> *mut c_void,
        arg: *mut c_void,
        e: *mut error,
    ) -> err_t,

    pub thread_join: unsafe extern "C" fn(os_self: *mut c_void, thread_self: *mut c_void),

    // Mutexes
    pub mutex_create:
        unsafe extern "C" fn(os_self: *mut c_void, dest: *mut i_mutex, e: *mut error) -> err_t,

    pub mutex_free: unsafe extern "C" fn(os_self: *mut c_void, mutex_self: *mut c_void),

    pub mutex_lock: unsafe extern "C" fn(mutex_self: *mut c_void),

    pub mutex_unlock: unsafe extern "C" fn(mutex_self: *mut c_void),

    // Condition variables
    pub cond_create:
        unsafe extern "C" fn(os_self: *mut c_void, dest: *mut i_cond, e: *mut error) -> err_t,

    pub cond_free: unsafe extern "C" fn(os_self: *mut c_void, cond_self: *mut c_void),

    pub cond_wait: unsafe extern "C" fn(cond_self: *mut c_void, mutex_self: *mut c_void),

    pub cond_timed_wait:
        unsafe extern "C" fn(cond_self: *mut c_void, mutex_self: *mut c_void, msec: u64),

    pub cond_signal: unsafe extern "C" fn(cond_self: *mut c_void),

    pub cond_broadcast: unsafe extern "C" fn(cond_self: *mut c_void),

    // Timer
    pub timer_create:
        unsafe extern "C" fn(os_self: *mut c_void, dest: *mut i_timer, e: *mut error) -> err_t,

    pub timer_free: unsafe extern "C" fn(os_self: *mut c_void, timer_self: *mut c_void),

    pub timer_now_ns: unsafe extern "C" fn(timer_self: *mut c_void) -> u64,
}

extern "C" {
    // System default
    pub fn system_os_create(mem: i_mem, dest: *mut i_os, e: *mut error) -> err_t;

    pub fn system_os_free(os: i_os);

    // Faulty version
    pub fn faulty_os_create(
        mem: i_mem,
        delegate: i_os,
        fail_percent: f32,
        dest: *mut i_os,
        e: *mut error,
    ) -> err_t;

    pub fn faulty_os_set_fail(os: i_os, fail_percent: f32);

    pub fn faulty_os_fail_after(os: i_os, count: c_int);

    pub fn faulty_num_os_calls(os: i_os) -> u32;

    pub fn faulty_os_reset(os: i_os);

    pub fn faulty_os_free(os: i_os);

    // Wrappers
    pub fn file_pread_all_expect(
        f: i_file,
        dest: *mut c_void,
        n: u64,
        offset: u64,
        e: *mut error,
    ) -> err_t;

    pub fn file_read_all_expect(f: i_file, dest: *mut c_void, nbytes: u64, e: *mut error) -> err_t;

    pub fn i_sleep_us(us: u64);
}
