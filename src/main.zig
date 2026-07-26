const std = @import("std");
const ns = @import("numstore.zig");

pub fn main() !void {
    std.debug.print("Hello world\n", .{});
    const db = try ns.Db.open("foo");
    defer db.close() catch {};

    try db.begin();
    _ = try db.execute("create b u32", null);
    try db.commit();
}
