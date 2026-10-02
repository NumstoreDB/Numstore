//! Raw declarations for the libnumstore functions nsserver calls.
//!
//! This is the hand-written equivalent of including numstore's headers:
//! one `extern` line per C function you use. Rust doesn't check these
//! against the C headers, so copy each signature carefully and keep this
//! file small; everything else goes through the safe wrapper in
//! `numstore.rs`.
//!
//! Type mapping: int → c_int, size_t → usize, uint64_t → u64,
//! double → f64, const char * → *const c_char, T * → *mut T.

#![allow(non_camel_case_types, dead_code)]

#[allow(unused_imports)]
use std::ffi::{c_char, c_int};

// Example of what this looks like. Replace the names and signatures with
// your real API from ../numstore/include, then uncomment.
//
// /// Opaque handle; Rust only ever holds a pointer to it.
// #[repr(C)]
// pub struct ns_db {
//     _data: [u8; 0],
//     _marker: std::marker::PhantomData<(*mut u8, std::marker::PhantomPinned)>,
// }
//
// unsafe extern "C" {
//     pub fn ns_open(path: *const c_char) -> *mut ns_db;
//     pub fn ns_close(db: *mut ns_db);
//     pub fn ns_put(db: *mut ns_db, key: u64, value: f64) -> c_int;
//     pub fn ns_get(db: *mut ns_db, key: u64, out: *mut f64) -> c_int;
//     pub fn ns_count(db: *mut ns_db) -> u64;
//     pub safe fn ns_version() -> *const c_char;
// }
