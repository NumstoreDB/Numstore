const std = @import("std");
const c = @cImport({
    @cInclude("smartfiles.h");
});
const e = @import("error.zig");
const stdtypes = @import("stdtypes.zig");
const sb_size = stdtypes.sb_size;
const b_size = stdtypes.b_size;
const t_size = stdtypes.t_size;

pub const Smfile = struct {
    ptr: *c.smfile_t,

    /// Open a new database.
    pub fn open(path: [*:0]const u8) e.Error!Smfile {
        const ptr = c.smfile_open(path) orelse return error.Io;
        return .{ .ptr = ptr };
    }

    /// Remove all data associated with a database
    /// including write ahead log and database file
    pub fn cleanup(path: [*:0]const u8) e.Error!void {
        return try e.check_int(c.smfile_cleanup(path));
    }

    /// Close the database
    pub fn close(self: Smfile) e.Error!void {
        return try e.check_int(c.smfile_close(self.ptr));
    }

    /// Crash the database
    pub fn crash(self: Smfile) e.Error!void {
        return try e.check_int(c.smfile_crash(self.ptr));
    }

    /// Insert a block of data into the database
    pub fn insert(
        self: Smfile,
        src: ?*const anyopaque,
        bofst: sb_size,
        slen: b_size,
    ) e.Error!b_size {
        return e.check_sb_size(c.smfile_insert(self.ptr, src, bofst, slen));
    }

    /// Read a block of data from the database
    pub fn read(
        self: Smfile,
        dest: ?*anyopaque,
        size: t_size,
        bofst: sb_size,
        stride: sb_size,
        nelem: b_size,
    ) e.Error!b_size {
        return e.check_sb_size(c.smfile_read(
            self.ptr,
            dest,
            size,
            bofst,
            stride,
            nelem,
        ));
    }

    /// Remove strided data from the database
    pub fn remove(
        self: Smfile,
        dest: ?*anyopaque,
        size: t_size,
        bofst: sb_size,
        stride: sb_size,
        nelem: b_size,
    ) e.Error!b_size {
        return try e.check_sb_size(c.smfile_remove(
            self.ptr,
            dest,
            size,
            bofst,
            stride,
            nelem,
        ));
    }

    /// Write data in the file
    pub fn write(
        self: Smfile,
        src: ?*const anyopaque,
        size: t_size,
        bofst: b_size,
        stride: sb_size,
        nelem: b_size,
    ) e.Error!b_size {
        return try e.check_sb_size(c.smfile_write(
            self.ptr,
            src,
            size,
            bofst,
            stride,
            nelem,
        ));
    }

    /// Begin a new transaction
    pub fn begin(self: Smfile) e.Error!void {
        return try e.check_int(c.smfile_begin(self.ptr));
    }

    /// Commit a transaction
    pub fn commit(self: Smfile) e.Error!void {
        return try e.check_int(c.smfile_commit(self.ptr));
    }

    /// Roll back a transaction
    pub fn rollback(self: Smfile) e.Error!void {
        return try e.check_int(c.smfile_rollback(self.ptr));
    }
};

const testing = @import("std").testing;
const test_helpers = @import("helpers.zig").test_helpers;

test "smoke test open" {
    try Smfile.cleanup("foo");
    const db = try Smfile.open("foo");
    try db.close();
}

test "smoke test cleanup close and crash" {
    const io = std.testing.io;

    // Expect no database to exist
    try Smfile.cleanup("foo");
    try testing.expect(!try test_helpers.fileExists(io, "foo"));
    try testing.expect(!try test_helpers.fileExists(io, "foo.wal"));

    // Open a new database
    var db = try Smfile.open("foo");
    try testing.expect(try test_helpers.fileExists(io, "foo"));
    try testing.expect(try test_helpers.fileExists(io, "foo.wal"));

    // Close it no more WAL
    try db.close();
    try testing.expect(try test_helpers.fileExists(io, "foo"));
    try testing.expect(!try test_helpers.fileExists(io, "foo.wal"));

    // Clean it up
    try Smfile.cleanup("foo");
    try testing.expect(!try test_helpers.fileExists(io, "foo"));
    try testing.expect(!try test_helpers.fileExists(io, "foo.wal"));

    // Open a new database
    db = try Smfile.open("foo");
    try testing.expect(try test_helpers.fileExists(io, "foo"));
    try testing.expect(try test_helpers.fileExists(io, "foo.wal"));

    // Crash it
    try db.crash();
    try testing.expect(try test_helpers.fileExists(io, "foo"));
    try testing.expect(try test_helpers.fileExists(io, "foo.wal"));

    // Clean it up all is gone
    try Smfile.cleanup("foo");
    try testing.expect(!try test_helpers.fileExists(io, "foo"));
    try testing.expect(!try test_helpers.fileExists(io, "foo.wal"));
}

test "sample 1 crud" {
    Smfile.cleanup("sample1_crud") catch {};
    const db = try Smfile.open("sample1_crud");

    var buf: [64]u8 = undefined;

    // Insert the initial sentence at offset 0.  len == 43
    const initial = "The quick brown fox jumps over the lazy dog";
    var n = try db.insert(@ptrCast(initial), 0, initial.len);
    try testing.expectEqual(@as(b_size, initial.len), n);

    // Insert " really" in the middle: at index 34, the space before "lazy".
    const adverb = " really"; // len 7
    n = try db.insert(adverb, 34, adverb.len);
    try testing.expectEqual(@as(b_size, adverb.len), n);

    // Read the whole array back.  now 43 + 7 == 50 bytes
    n = try db.read(&buf, 1, 0, 1, c.SMF_END);
    try testing.expectEqualStrings(
        "The quick brown fox jumps over the really lazy dog",
        buf[0..@intCast(n)],
    );

    // Overwrite "fox" (indices 16..18) with "cat".
    const cat = "cat";
    n = try db.write(cat, 1, 16, 1, cat.len);
    try testing.expectEqual(@as(b_size, cat.len), n); // 3

    n = try db.read(&buf, 1, 0, 1, c.SMF_END);
    try testing.expectEqualStrings(
        "The quick brown cat jumps over the really lazy dog",
        buf[0..@intCast(n)],
    );

    // Remove the 7 bytes of " really" (indices 34..40), capturing the eviction.
    var evicted: [8]u8 = undefined;
    n = try db.remove(&evicted, 1, 34, 1, 7);
    try testing.expectEqual(@as(b_size, 7), n);
    try testing.expectEqualStrings(" really", evicted[0..@intCast(n)]);

    // Read the final result.  50 - 7 == 43 bytes
    n = try db.read(&buf, 1, 0, 1, c.SMF_END);
    try testing.expectEqualStrings(
        "The quick brown cat jumps over the lazy dog",
        buf[0..@intCast(n)],
    );

    try db.close();
}

test "sample 2 txn" {
    Smfile.cleanup("sample2_txn") catch {};
    const db = try Smfile.open("sample2_txn");

    // header[0..7] = 1, body[8..71] = 0..63, footer[72..79] = 99
    try db.begin();
    var header: [8]u8 = undefined;
    var body: [64]u8 = undefined;
    var footer: [8]u8 = undefined;
    @memset(&header, 1);
    for (&body, 0..) |*v, i| v.* = @intCast(i);
    @memset(&footer, 99);
    _ = try db.insert(&header, 0, header.len);
    _ = try db.insert(&body, 8, body.len);
    _ = try db.insert(&footer, 72, footer.len);
    try db.commit();

    // Rolled-back transaction: overwrite everything with 0x00, then undo
    try db.begin();
    var zeros: [80]u8 = undefined;
    @memset(&zeros, 0);
    _ = try db.write(&zeros, 1, 0, 1, zeros.len);
    try db.rollback();

    // Bytes [68..79] must be unchanged by the rolled-back write.
    // [68..71] are body[60..63] == 60,61,62,63; [72..79] are footer == 99.
    var verify: [12]u8 = undefined;
    var n = try db.read(&verify, 1, 68, 1, 12);
    try testing.expectEqual(@as(b_size, 12), n);
    try testing.expectEqualSlices(u8, &.{
        60, 61, 62, 63, 99, 99, 99, 99, 99, 99, 99, 99,
    }, verify[0..@intCast(n)]);

    // ---- Committed transaction: append 4 bytes of 0xCC at [80..83] ----
    try db.begin();
    var extra: [4]u8 = undefined;
    @memset(&extra, 0xCC);
    _ = try db.insert(&extra, 80, extra.len);
    try db.commit();

    // ---- Rolled-back transaction: try to remove those 4 bytes, then undo ----
    try db.begin();
    var scratch: [4]u8 = undefined; // wrapper needs a dest; C passed NULL here
    _ = try db.remove(&scratch, 1, 80, 1, 4);
    try db.rollback();

    // The append survived; the removal was undone. [80..83] == 0xCC.
    var tail: [4]u8 = undefined;
    n = try db.read(&tail, 1, 80, 1, 4);
    try testing.expectEqual(@as(b_size, 4), n);
    try testing.expectEqualSlices(u8, &.{ 0xCC, 0xCC, 0xCC, 0xCC }, tail[0..@intCast(n)]);

    try db.close();
}

test "sample 4 stride" {
    Smfile.cleanup("sample4_stride") catch {};
    const db = try Smfile.open("sample4_stride");

    const fsz = @sizeOf(f32);

    // Insert 16 contiguous floats: data[i] == i
    var data: [16]f32 = undefined;
    for (&data, 0..) |*v, i| v.* = @floatFromInt(i);
    _ = try db.insert(@ptrCast(&data), 0, @sizeOf(@TypeOf(data)));

    // Read every other float (byte offset 0, stride 2, 8 elems): 0 2 4 6 ... 14
    var evens: [8]f32 = undefined;
    var n = try db.read(@ptrCast(&evens), fsz, 0, 2, 8);
    try testing.expectEqual(@as(b_size, 8), n);
    for (evens[0..@intCast(n)], 0..) |v, i| {
        try testing.expectEqual(@as(f32, @floatFromInt(i * 2)), v);
    }

    // Write -1 starting at byte offset 4 (element 1), stride 2, 8 elems:
    // overwrites the odd positions 1,3,5,...,15.
    var neg: [8]f32 = undefined;
    @memset(&neg, -1.0);
    _ = try db.write(@ptrCast(&neg), fsz, 4, 2, 8);

    // Read back all 16 (stride 1): 0,-1,2,-1,4,-1,...,14,-1
    var readback: [16]f32 = undefined;
    n = try db.read(@ptrCast(&readback), fsz, 0, 1, 16);
    try testing.expectEqual(@as(b_size, 16), n);
    for (readback[0..@intCast(n)], 0..) |v, i| {
        const exp: f32 = if (i % 2 == 0) @floatFromInt(i) else -1.0;
        try testing.expectEqual(exp, v);
    }

    // Remove 8 elements (byte offset 0, stride 2): pulls out the even positions
    // 0,2,4,...,14, capturing them.
    var removed: [8]f32 = undefined;
    n = try db.remove(@ptrCast(&removed), fsz, 0, 2, 8);
    try testing.expectEqual(@as(b_size, 8), n);
    for (removed[0..@intCast(n)], 0..) |v, i| {
        try testing.expectEqual(@as(f32, @floatFromInt(i * 2)), v);
    }

    // What remains are the 8 odd positions, all -1.
    n = try db.read(@ptrCast(&readback), fsz, 0, 1, 8);
    try testing.expectEqual(@as(b_size, 8), n);
    for (readback[0..@intCast(n)]) |v| {
        try testing.expectEqual(@as(f32, -1.0), v);
    }

    try db.close();
}

test "sample 5 durability" {
    const PATH = "sample5_durability";
    Smfile.cleanup(PATH) catch {};

    // Phase 1 - baseline: 10 'A's, committed cleanly.
    {
        const db = try Smfile.open(PATH);
        try db.begin();
        _ = try db.insert("AAAAAAAAAA", 0, 10);
        try db.commit();
        try db.close();
    }

    // Phase 2 - insert "BB" at 3, commit, clean exit. Must survive.
    {
        const db = try Smfile.open(PATH);
        try db.begin();
        _ = try db.insert("BB", 3, 2);
        try db.commit();
        try db.close();
    }

    // Phase 3 - insert "CC" at 7, commit, then CRASH.
    // Committed before the crash -> must be replayed on next open.
    {
        const db = try Smfile.open(PATH);
        try db.begin();
        _ = try db.insert("CC", 7, 2);
        try db.commit();
        try db.crash(); // no close: WAL left committed-but-unflushed
    }

    // Phase 4 - insert "DD" at 11, NO commit, then CRASH.
    // Never committed -> must be discarded on next open.
    {
        const db = try Smfile.open(PATH); // replays phase 3's CC
        try db.begin();
        _ = try db.insert("DD", 11, 2);
        try db.crash(); // no commit, no close
    }

    // Phase 5 - insert "EE" at 5, explicit rollback, clean exit.
    // Rolled back -> must leave no trace.
    {
        const db = try Smfile.open(PATH);
        try db.begin();
        _ = try db.insert("EE", 5, 2);
        try db.rollback();
        try db.close();
    }

    // Phase 6 - reopen fresh and verify the durability guarantees held.
    const db = try Smfile.open(PATH);
    var buf: [64]u8 = undefined;

    // Full contents: ph2 (BB) + ph3 (CC) visible; ph4 (DD) + ph5 (EE) gone.
    // insert shifts rather than overwrites, so the file grew 10 -> 14 bytes:
    //   AAAAAAAAAA -> AAA[BB]AAAAAAA -> AAABBAA[CC]AAAAA
    var n = try db.read(&buf, 1, 0, 1, 14);
    try testing.expectEqual(@as(b_size, 14), n);
    try testing.expectEqualStrings("AAABBAACCAAAAA", buf[0..@intCast(n)]);

    // ph2 - committed, clean exit
    n = try db.read(&buf, 1, 3, 1, 2);
    try testing.expectEqualStrings("BB", buf[0..@intCast(n)]);

    // ph3 - committed then crashed (survives via WAL replay)
    n = try db.read(&buf, 1, 7, 1, 2);
    try testing.expectEqualStrings("CC", buf[0..@intCast(n)]);

    // ph4 - uncommitted + crashed (discarded; slot is still 'A')
    n = try db.read(&buf, 1, 11, 1, 1);
    try testing.expectEqualStrings("A", buf[0..@intCast(n)]);

    // ph5 - rolled back (discarded; slot is still 'A')
    n = try db.read(&buf, 1, 5, 1, 1);
    try testing.expectEqualStrings("A", buf[0..@intCast(n)]);

    try db.close();
}
