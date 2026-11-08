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
        assert!(fname.ends_with('\0'));

        Database {
            handle: unsafe { ns_open(fname.as_ptr()) },
        }
    }
}
