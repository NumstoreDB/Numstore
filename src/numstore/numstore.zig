const std = @import("std");
const testing = std.testing;
const c = @cImport({
    @cInclude("numstore/numstore.h");
});

const e = @import("../core/error.zig");
const sb_size = @import("../core/stdtypes.zig").sb_size;
const b_size = @import("../core/stdtypes.zig").b_size;
const t_size = @import("../core/stdtypes.zig").t_size;
const test_helpers = @import("../core/helpers.zig").test_helpers;

pub const Db = struct {
    ptr: *c.nsdb_t,

    /// Open (creating if needed) a numstore database.
    pub fn open(path: [*:0]const u8) e.Error!Db {
        const ptr = c.nsdb_open(path) orelse return error.Io;
        return .{ .ptr = ptr };
    }

    /// Remove all on-disk resources for `path`.
    pub fn cleanup(path: [*:0]const u8) e.Error!void {
        return try e.check_int(c.nsdb_cleanup(path));
    }

    /// Close the database
    pub fn close(self: Db) e.Error!void {
        return e.check_int(c.nsdb_close(self.ptr));
    }

    /// Crash the database
    pub fn crash(self: Db) e.Error!void {
        return try e.check_int(c.nsdb_crash(self.ptr));
    }

    /// Begin a new transaction
    pub fn begin(self: Db) e.Error!void {
        return try e.check_int(c.nsdb_begin(self.ptr));
    }

    /// Commit a transaction
    pub fn commit(self: Db) e.Error!void {
        return try e.check_int(c.nsdb_commit(self.ptr));
    }

    /// Roll a transaction back
    pub fn rollback(self: Db) e.Error!void {
        return try e.check_int(c.nsdb_rollback(self.ptr));
    }

    /// Execute a query
    pub fn execute(self: Db, query: [*:0]const u8, data: ?*anyopaque) e.Error!u64 {
        return try e.check_sb_size(c.nsdb_execute(self.ptr, query, data));
    }
};

pub const Var = struct {
    ptr: *c.nsdb_var_t,

    /// Number of elements addressable through this handle.
    pub fn len(self: Var) u64 {
        return c.nsdb_var_len(self.ptr);
    }

    /// Release the handle and any resources it owns.
    pub fn free(self: Var) void {
        c.nsdb_var_free(self.ptr);
    }
};

test "smoke test open" {
    try Db.cleanup("foo");
    const db = try Db.open("foo");
    try db.close();
}

test "smoke test cleanup close and crash" {
    const io = std.testing.io;

    // Expect no database to exist
    try Db.cleanup("foo");
    try testing.expect(!try test_helpers.fileOrDirExists(io, "foo"));
    try testing.expect(!try test_helpers.fileOrDirExists(io, "foo.wal"));

    // Open a new database
    var db = try Db.open("foo");
    try testing.expect(try test_helpers.fileOrDirExists(io, "foo"));
    try testing.expect(try test_helpers.fileOrDirExists(io, "foo.wal"));

    // Close it no more WAL
    try db.close();
    try testing.expect(try test_helpers.fileOrDirExists(io, "foo"));
    try testing.expect(!try test_helpers.fileOrDirExists(io, "foo.wal"));

    // Clean it up
    try Db.cleanup("foo");
    try testing.expect(!try test_helpers.fileOrDirExists(io, "foo"));
    try testing.expect(!try test_helpers.fileOrDirExists(io, "foo.wal"));

    // Open a new database
    db = try Db.open("foo");
    try testing.expect(try test_helpers.fileOrDirExists(io, "foo"));
    try testing.expect(try test_helpers.fileOrDirExists(io, "foo.wal"));

    // Crash it
    try db.crash();
    try testing.expect(try test_helpers.fileOrDirExists(io, "foo"));
    try testing.expect(try test_helpers.fileOrDirExists(io, "foo.wal"));

    // Clean it up all is gone
    try Db.cleanup("foo");
    try testing.expect(!try test_helpers.fileOrDirExists(io, "foo"));
    try testing.expect(!try test_helpers.fileOrDirExists(io, "foo.wal"));
}

test "sample 1" {
    var src: [200]u32 = undefined;
    var dest: [200]u32 = undefined;

    Db.cleanup("test") catch {};
    const db = try Db.open("test");

    _ = try db.execute("create example u32", null);

    // Just a growing int: src[i] == i
    for (&src, 0..) |*v, i| {
        v.* = @intCast(i);
    }
    // Insert src into db
    var n = try db.execute("insert example 0 200", @ptrCast(&src));
    try testing.expectEqual(200, n);

    // Should be every third element: dest[i] == i*3
    n = try db.execute("read example[0:-10:3]", @ptrCast(&dest));
    try testing.expectEqual((200 - 10 - 1) / 3 + 1, n);
    for (dest[0..n], 0..) |v, i| {
        try testing.expectEqual(@as(u32, @intCast(i * 3)), v);
    }

    // Removed every second element in [0:-10]: dest[i] == i*2
    n = try db.execute("remove example[0:-10:2] blimit 800", @ptrCast(&dest));
    const removed = (200 - 10 - 1) / 2 + 1; // 95
    try testing.expectEqual(removed, n);
    for (dest[0..n], 0..) |v, i| {
        try testing.expectEqual(@as(u32, @intCast(i * 2)), v);
    }

    // Read everything left after the remove.
    n = try db.execute("read example[0:] blimit 800", @ptrCast(&dest));
    try testing.expectEqual(200 - removed, n);
    const kept_in_range = (200 - 10) - removed; // 95 odd survivors

    // 1, 3, ..., 189
    for (dest[0..kept_in_range], 0..) |v, i| {
        try testing.expectEqual(@as(u32, @intCast(i * 2 + 1)), v);
    }

    // 190 .. 199
    for (dest[kept_in_range..n], 0..) |v, i| {
        try testing.expectEqual(@as(u32, @intCast(200 - 10 + i)), v);
    }

    // Overwrite all remaining entries with src[0..n] = 0,1,2,...,n-1
    n = try db.execute("write example[0::] blimit 800", @ptrCast(&src));
    try testing.expectEqual(200 - removed, n);

    n = try db.execute("read example[0:] blimit 800", @ptrCast(&dest));
    try testing.expectEqual(200 - removed, n);
    for (dest[0..n], 0..) |v, i| {
        try testing.expectEqual(@as(u32, @intCast(i)), v);
    }

    try db.close();
}

test "sample 2" {
    const Example = extern struct {
        a: f32,
        b: i32,
        d: [5][10]u32,
    };
    var src: [200]Example = undefined;
    var dest: [200]Example = undefined;

    Db.cleanup("sample2") catch {};
    const db = try Db.open("sample2");
    _ = try db.execute(
        \\create example struct {
        \\  a f32,
        \\  b i32,
        \\  d [5][10] u32
        \\}
    , null);

    // src[i] {
    //  .a = i,
    //  .b = i+1,
    //  .d[r][col] = i + r*10 + col
    // }
    for (&src, 0..) |*v, i| {
        v.a = @floatFromInt(i);
        v.b = @intCast(i + 1);
        for (0..5) |r| {
            for (0..10) |col| {
                v.d[r][col] = @intCast(i + r * 10 + col);
            }
        }
    }

    const expectElem = struct {
        fn f(v: Example, si: usize) !void {
            try testing.expectEqual(@as(f32, @floatFromInt(si)), v.a);
            try testing.expectEqual(@as(i32, @intCast(si + 1)), v.b);
            for (0..5) |r| {
                for (0..10) |col| {
                    try testing.expectEqual(@as(u32, @intCast(si + r * 10 + col)), v.d[r][col]);
                }
            }
        }
    }.f;

    const dest_bytes = @sizeOf(@TypeOf(dest)); // 200 * 208
    const src_bytes = @sizeOf(@TypeOf(src));

    // Insert src into db
    var n = try db.execute("insert example 0 200", @ptrCast(&src));
    try testing.expectEqual(200, n);

    // Every third element: dest[i] == src[i*3]
    n = try db.execute(
        comptime std.fmt.comptimePrint("read example[0:-10:3] blimit {d}", .{dest_bytes}),
        @ptrCast(&dest),
    );
    try testing.expectEqual((200 - 10 - 1) / 3 + 1, n);
    for (dest[0..n], 0..) |v, i| {
        try expectElem(v, i * 3);
    }

    // Remove every second element in [0:-10]: dest[i] == src[i*2]
    n = try db.execute(
        comptime std.fmt.comptimePrint("remove example[0:-10:2] blimit {d}", .{dest_bytes}),
        @ptrCast(&dest),
    );
    const removed = (200 - 10 - 1) / 2 + 1; // 95
    try testing.expectEqual(removed, n);
    for (dest[0..n], 0..) |v, i| {
        try expectElem(v, i * 2);
    }

    // Read everything left after the remove (two passes, remove stopped at -10):
    n = try db.execute(
        comptime std.fmt.comptimePrint("read example[0:] blimit {d}", .{dest_bytes}),
        @ptrCast(&dest),
    );
    try testing.expectEqual(200 - removed, n);
    const kept_in_range = (200 - 10) - removed; // 95 odd survivors

    // 1, 3, ..., 189
    for (dest[0..kept_in_range], 0..) |v, i| {
        try expectElem(v, i * 2 + 1);
    }

    // Pass 2: src indices 190 .. 199
    for (dest[kept_in_range..n], 0..) |v, i| {
        try expectElem(v, 200 - 10 + i);
    }

    // Overwrite all remaining entries with src[0..n] -> dest[i] == src[i]
    n = try db.execute(
        comptime std.fmt.comptimePrint("write example[0::] blimit {d}", .{src_bytes}),
        @ptrCast(&src),
    );
    try testing.expectEqual(200 - removed, n);
    n = try db.execute(
        comptime std.fmt.comptimePrint("read example[0:] blimit {d}", .{dest_bytes}),
        @ptrCast(&dest),
    );
    try testing.expectEqual(200 - removed, n);
    for (dest[0..n], 0..) |v, i| {
        try expectElem(v, i);
    }

    try db.close();
}
