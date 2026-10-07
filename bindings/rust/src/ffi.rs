#[allow(non_camel_case_types)]
use std::ffi::{c_char, c_int, c_void};
use std::marker::{PhantomData, PhantomPinned};

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

pub const NS_PAGE_SIZE: usize = 4096;
pub const MEMORY_PAGE_LEN: usize = 4096;
pub const WAL_BUFFER_CAP: usize = 1_048_576;
pub const NS_END: sb_size = i64::MAX;
pub const SMF_END: sb_size = i64::MAX;
pub const PGNO_NULL: pgno = u64::MAX;
pub const LSN_NULL: lsn = u64::MAX;
pub const WLH_NULL: wlh = u8::MAX;

// Opaque
macro_rules! opaque {
    ($($(#[$m:meta])* $name:ident;)*) => {$(
        $(#[$m])*
        #[repr(C)]
        pub struct $name {
            _data: [u8; 0],
            _marker: PhantomData<(*mut u8, PhantomPinned)>,
        }
    )*};
}

opaque! {
    nsdb_t;
    txn_t;
    nsdb_var_t;
    nsdb_plan_t;
}

#[link(name = "numstore")]
extern "C" {
    // Lifecycle
    pub fn ns_open(path: *const c_char) -> *mut nsdb_t;
    pub fn ns_smfile_open(path: *const c_char) -> *mut nsdb_t;
    pub fn ns_cleanup(path: *const c_char) -> c_int;
    pub fn ns_close(ns: *mut nsdb_t) -> c_int;
    pub fn ns_crash(ns: *mut nsdb_t) -> c_int;

    // Variable getters
    pub fn ns_var_len(var: *mut nsdb_var_t) -> b_size;
    pub fn ns_var_free(var: *mut nsdb_var_t);

    // Error handling
    pub fn ns_strerror(ns: *mut nsdb_t) -> *const c_char;
    pub fn ns_plan_strerror(plan: *mut nsdb_plan_t) -> *const c_char;

    // Transactions
    pub fn ns_begin(ns: *mut nsdb_t) -> *mut txn_t;
    pub fn ns_commit(ns: *mut nsdb_t, txn: *mut txn_t) -> c_int;
    pub fn ns_rollback(ns: *mut nsdb_t, txn: *mut txn_t) -> c_int;

    // Plans
    pub fn ns_plan_fcreate(db: *mut nsdb_t, fmt: *const c_char, ...) -> *mut nsdb_plan_t;
    pub fn ns_plan_free(plan: *mut nsdb_plan_t);

    // Execute
    pub fn ns_execute(db: *mut nsdb_t, tx: *mut txn_t, fmt: *const c_char, ...) -> c_int;
    pub fn ns_plan_execute(plan: *mut nsdb_plan_t, tx: *mut txn_t) -> c_int;

    // Get var
    pub fn ns_get_var(
        db: *mut nsdb_t,
        tx: *mut txn_t,
        query: *const c_char,
        ...
    ) -> *mut nsdb_var_t;
    pub fn ns_plan_get_var(plan: *mut nsdb_plan_t, tx: *mut txn_t) -> *mut nsdb_var_t;

    // Read
    pub fn ns_read(
        db: *mut nsdb_t,
        txn: *mut txn_t,
        dest: *mut c_void,
        dlen: b_size,
        fmt: *const c_char,
        ...
    ) -> sb_size;
    pub fn ns_plan_read(
        plan: *mut nsdb_plan_t,
        txn: *mut txn_t,
        dest: *mut c_void,
        dlen: b_size,
    ) -> sb_size;

    // Write
    pub fn ns_write(
        db: *mut nsdb_t,
        txn: *mut txn_t,
        src: *const c_void,
        dlen: b_size,
        fmt: *const c_char,
        ...
    ) -> sb_size;
    pub fn ns_plan_write(
        plan: *mut nsdb_plan_t,
        tx: *mut txn_t,
        src: *const c_void,
        dlen: b_size,
    ) -> sb_size;

    // Malloc'd read (free with libc `free`)
    pub fn ns_malloc(
        db: *mut nsdb_t,
        txn: *mut txn_t,
        dlen: *mut b_size,
        fmt: *const c_char,
        ...
    ) -> *mut c_void;
    pub fn ns_plan_malloc(plan: *mut nsdb_plan_t, tx: *mut txn_t, dlen: *mut b_size)
    -> *mut c_void;

    // Smart files
    pub fn ns_smfile_size(smf: *mut nsdb_t, tx: *mut txn_t) -> sb_size;
    pub fn ns_smfile_insert(
        smf: *mut nsdb_t,
        tx: *mut txn_t,
        src: *const c_void,
        bofst: sb_size,
        slen: b_size,
    ) -> sb_size;
    pub fn ns_smfile_write(
        smf: *mut nsdb_t,
        tx: *mut txn_t,
        src: *const c_void,
        size: t_size,
        bofst: sb_size,
        stride: sb_size,
        nelem: b_size,
    ) -> sb_size;
    pub fn ns_smfile_read(
        smf: *mut nsdb_t,
        tx: *mut txn_t,
        dest: *mut c_void,
        size: t_size,
        bofst: sb_size,
        stride: sb_size,
        nelem: b_size,
    ) -> sb_size;
    pub fn ns_smfile_remove(
        smf: *mut nsdb_t,
        tx: *mut txn_t,
        dest: *mut c_void,
        size: t_size,
        bofst: sb_size,
        stride: sb_size,
        nelem: b_size,
    ) -> sb_size;
}

const _: () = {
    assert!(size_of::<sys::nsdb_t>() == 0);
    assert!(size_of::<*mut sys::nsdb_t>() == size_of::<*mut c_void>());
    assert!(size_of::<Option<NonNull<sys::nsdb_t>>>() == size_of::<*mut c_void>());
};
