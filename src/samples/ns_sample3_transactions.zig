const std = @import("std");
const numstore = @import("numstore");

const Db = numstore.numstore.Db;

const Example = extern struct {
    a: f32,
    b: i32,
    d: [5][10]u32,
};

var src: [200]Example = undefined;

pub fn main() !void {
    var buf: [256]u8 = undefined;

    Db.cleanup("sample1_crud") catch {};
    const db = try Db.open("sample1_crud");

    _ = try db.execute(
        \\create example struct {
        \\  a f32,
        \\  b i32,
        \\  d [5][10] u32
        \\}
    , null);

    for (&src, 0..) |*e, i| {
        e.a = @floatFromInt(i);
        e.b = @intCast(i + 1);
        for (0..5) |r| {
            for (0..10) |col| {
                e.d[r][col] = @intCast(i + r * 10 + col);
            }
        }
    }

    _ = try db.executeFmt(&buf, @ptrCast(&src), "insert example 0 {d}", .{200});

    var n: usize = @intCast(try db.executeFmt(
        &buf,
        @ptrCast(&dest),
        "read example[0:-10:3] blimit {d}",
        .{@sizeOf(@TypeOf(dest))},
    ));
    printExample("Read elements: ", dest[0..n]);

    n = @intCast(try db.executeFmt(
        &buf,
        @ptrCast(&dest),
        "remove example[0:-10:2] blimit {d}",
        .{@sizeOf(@TypeOf(dest))},
    ));
    printExample("Removed elements: ", dest[0..n]);

    n = @intCast(try db.executeFmt(
        &buf,
        @ptrCast(&dest),
        "read example[0:] blimit {d}",
        .{@sizeOf(@TypeOf(dest))},
    ));
    printExample("After Remove: ", dest[0..n]);

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
    printExample("After write: ", dest[0..n]);

    try db.close();
}

fn printExample(label: []const u8, ex: []const Example) void {
    const size = ex.len;
    const show = @min(size, 10);
    std.debug.print("{s}: examples ({d}):\n", .{ label, size });
    for (ex[0..show], 0..) |e, i| {
        std.debug.print(
            "{s}:   [{d}] a={d}  b={d}  d=[[{d}, {d} ...], [{d}, {d} ...], ...]\n",
            .{ label, i, e.a, e.b, e.d[0][0], e.d[0][1], e.d[1][0], e.d[1][1] },
        );
    }
    if (size > show) {
        std.debug.print("{s}:   ... ({d} more)\n", .{ label, size - show });
    }
}
