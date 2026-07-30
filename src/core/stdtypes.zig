pub const t_size = u32;
pub const st_size = i32;
pub const p_size = u32;
pub const sp_size = i32;
pub const b_size = u64;
pub const sb_size = i64;
pub const pgno = u64;
pub const spgno = i64;
pub const txid = u64;
pub const stxid = i64;
pub const lsn = u64;
pub const slsn = i64;

/// Used for various build operations
/// We put a bound on file sizes
/// to ensure we don't limit the read
/// stream
pub const max_file_size = 8192 * 100;
