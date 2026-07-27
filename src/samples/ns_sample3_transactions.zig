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
    Db.cleanup("sample1_crud") catch {};
    const db = try Db.open("sample1_crud");

    _ = try db.begin();

    _ = try db.execute(
        \\create example struct {
        \\  a f32,
        \\  b i32,
        \\  d [5][10] u32
        \\}
    , null);

    _ = try db.commit();

    try db.close();
}
