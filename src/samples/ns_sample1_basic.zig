const std = @import("std");
const numstore = @import("numstore");

const Db = numstore.Db;

var src: [200]u32 = undefined;
var dest: [200]u32 = undefined;

pub fn main() !void {
    var buf: [256]u8 = undefined;

    Db.cleanup("sample1_crud") catch {};
    const db = try Db.open("sample1_crud");

    _ = try db.execute("create example u32", null);

    for (&src, 0..) |*e, i| {
        e.* = @intCast(i);
    }

    _ = try db.executeFmt(&buf, @ptrCast(&src), "insert example 0 {d}", .{200});

    var n: usize = @intCast(try db.executeFmt(
        &buf,
        @ptrCast(&dest),
        "read example[0:-10:3] blimit {d}",
        .{@sizeOf(@TypeOf(dest))},
    ));
    printExample(dest[0..n]);

    n = @intCast(try db.executeFmt(
        &buf,
        @ptrCast(&dest),
        "remove example[0:-10:2] blimit {d}",
        .{@sizeOf(@TypeOf(dest))},
    ));
    printExample(dest[0..n]);

    n = @intCast(try db.executeFmt(
        &buf,
        @ptrCast(&dest),
        "read example[0:] blimit {d}",
        .{@sizeOf(@TypeOf(dest))},
    ));
    printExample(dest[0..n]);

    _ = try db.executeFmt(
        &buf,
        @ptrCast(&src),
        "write example[0::] blimit {d}",
        .{@sizeOf(@TypeOf(src))},
    );
    n = @intCast(try db.executeFmt(
        &buf,
        @ptrCast(&dest),
        "read example[0:] blimit {d}",
        .{@sizeOf(@TypeOf(dest))},
    ));
    printExample(dest[0..n]);

    try db.close();
}

fn printExample(ex: []const u32) void {
    const size = ex.len;
    const show = @min(size, 10);

    std.debug.print("[", .{});
    for (ex[0..show]) |e| {
        std.debug.print("{d} ", .{e});
    }
    std.debug.print("]\n", .{});
}
