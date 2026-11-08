use std::ffi::CString;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

struct Database {
    handle: *mut nsdb_t,
}

pub fn rs_ns_open(fname: &str) -> *mut nsdb_t {
    let c_fname = CString::new(fname).expect("Error");

    unsafe {
        return ns_open(c_fname.as_ptr());
    };
}

impl Database {
    pub fn new(fname: &str) -> Database {
        // rs_ns_open adds the NUL terminator; CString::new rejects a name
        // that already contains one.
        Database {
            handle: rs_ns_open(fname),
        }
    }
}
