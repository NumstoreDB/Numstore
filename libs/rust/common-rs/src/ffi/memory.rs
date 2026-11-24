use core::ffi::c_void;

use crate::ffi::error::{err_t, error};

#[repr(C)]
#[derive(Copy, Clone)]
pub struct i_mem {
    pub table: *const i_mem_vtable,
    pub self_: *mut c_void,
}

#[repr(C)]
pub struct i_mem_vtable {
    pub malloc: unsafe extern "C" fn(
        self_: *mut c_void,
        nelem: u32,
        size: u32,
        e: *mut error,
    ) -> *mut c_void,

    pub calloc: unsafe extern "C" fn(
        self_: *mut c_void,
        nelem: u32,
        size: u32,
        e: *mut error,
    ) -> *mut c_void,

    pub realloc: unsafe extern "C" fn(
        self_: *mut c_void,
        ptr: *mut c_void,
        nelem: u32,
        size: u32,
        e: *mut error,
    ) -> *mut c_void,

    pub free: unsafe extern "C" fn(self_: *mut c_void, v: *mut c_void),
}

extern "C" {
    /// The system default allocator (malloc/calloc/realloc/free).
    pub fn default_mem() -> i_mem;

    // Faulty memory (fault injection for tests)
    pub fn faulty_mem_create(
        delegate: i_mem,
        fail_percent: f32,
        dest: *mut i_mem,
        e: *mut error,
    ) -> err_t;
    pub fn faulty_mem_set_fail(mem: i_mem, fail_percent: f32);
    pub fn faulty_mem_fail_after(mem: i_mem, count: core::ffi::c_int);
    pub fn faulty_mem_reset(mem: i_mem);
    pub fn faulty_num_mallocs(mem: i_mem) -> u32;
    pub fn faulty_mem_free(mem: i_mem);
}
