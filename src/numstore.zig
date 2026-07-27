const std = @import("std");
const c = @cImport({
    @cInclude("numstore.h");
});

pub const nsdb_t = opaque {};
pub const nsdb_var_t = opaque {};

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

pub const NS_END: i64 = std.math.maxInt(i64);
pub const SMF_END: i64 = std.math.maxInt(i64);

pub const Error = error{
    OpenFailed,
    Nsdb,
    QueryTooLong,
};

pub const Db = struct {
    ptr: *c.nsdb_t,

    /// Wrap a raw `*nsdb_t` (e.g. one handed back through a C boundary).
    pub fn fromPtr(ptr: *c.nsdb_t) Db {
        return .{ .ptr = ptr };
    }

    /// Open (creating if needed) a numstore database.
    pub fn open(path: [*:0]const u8) Error!Db {
        return .{ .ptr = c.nsdb_open(path) orelse return error.OpenFailed };
    }

    /// Remove all on-disk resources for `path`.
    pub fn cleanup(path: [*:0]const u8) Error!void {
        if (c.nsdb_cleanup(path) < 0) return error.Nsdb;
    }

    /// Close the database
    pub fn close(self: Db) Error!void {
        if (c.nsdb_close(self.ptr) < 0) return error.Nsdb;
    }

    /// Crash the database
    pub fn crash(self: Db) Error!void {
        if (c.nsdb_crash(self.ptr) < 0) return error.Nsdb;
    }

    // Transaction control

    pub fn begin(self: Db) Error!void {
        if (c.nsdb_begin(self.ptr) < 0) return error.Nsdb;
    }

    pub fn commit(self: Db) Error!void {
        if (c.nsdb_commit(self.ptr) < 0) return error.Nsdb;
    }

    pub fn rollback(self: Db) Error!void {
        if (c.nsdb_rollback(self.ptr) < 0) return error.Nsdb;
    }

    /// Run `body` inside begin/commit, rolling back automatically on error.
    /// `body` is any callable returning `!void`.
    pub fn transaction(self: Db, body: anytype) !void {
        try self.begin();
        errdefer self.rollback() catch {};
        try body();
        try self.commit();
    }

    // Query execution

    /// Execute a query. `data` is the caller-owned buffer used by
    /// insert / read / write / remove; pass `null` for queries with no payload.
    /// Returns the number of elements touched (0 for non-data queries).
    pub fn execute(self: Db, query: [*:0]const u8, data: ?*anyopaque) Error!u64 {
        const n = c.nsdb_execute(self.ptr, query, data);
        if (n < 0) return error.Nsdb;
        return @intCast(n);
    }

    /// Build a query with Zig's `std.fmt` into `buf`, then execute it.
    ///
    /// This intentionally sidesteps `nsdb_fexecute`'s printf format string:
    /// numstore receives the already-rendered query, so use Zig placeholders
    /// (`{d}`, `{s}`, ...) rather than `%d`. For the genuine C variadic path,
    /// call `c.nsdb_fexecute` directly.
    pub fn executeFmt(
        self: Db,
        buf: []u8,
        data: ?*anyopaque,
        comptime fmt: []const u8,
        args: anytype,
    ) Error!u64 {
        const query = std.fmt.bufPrintZ(buf, fmt, args) catch return error.QueryTooLong;
        return self.execute(query, data);
    }

    // Error reporting

    /// Current error message, or `null` if none is set. The returned slice is
    /// owned by numstore and valid until the next call on this handle.
    pub fn lastError(self: Db) ?[:0]const u8 {
        const s = c.nsdb_strerror(self.ptr) orelse return null;
        return std.mem.span(s);
    }

    /// Print the current error to stderr, prefixed like `perror(3)`.
    pub fn perror(self: Db, prefix: [*:0]const u8) void {
        _ = c.nsdb_perror(self.ptr, prefix);
    }
};

pub const Var = struct {
    ptr: *c.nsdb_var_t,

    pub fn fromPtr(ptr: *c.nsdb_var_t) Var {
        return .{ .ptr = ptr };
    }

    /// Number of elements addressable through this handle.
    pub fn len(self: Var) u64 {
        return c.nsdb_var_len(self.ptr);
    }

    /// Release the handle and any resources it owns.
    pub fn free(self: Var) void {
        c.nsdb_var_free(self.ptr);
    }
};
