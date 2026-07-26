// Copyright 2026 Theo Lincke
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

const std = @import("std");
const numstore = @import("numstore.zig");
const Db = numstore.Db;

// C ABI layout. Every field here is 4-byte sized/aligned, so `extern struct`
// already matches the C `__attribute__((packed))` version byte-for-byte — no
// Zig `packed struct` (bit-level) needed.
const Example = extern struct {
    a: f32,
    b: i32,
    d: [5][10]u32,
};

// Source and destination buffers.
var src: [200]Example = undefined;
var dest: [200]Example = undefined;

/// This example shows basic first-class operations on smart files:
///   1. Insert (insert data into the middle of an array)
///   2. Read   (normal read)
///   3. Write  (overwrite data in the middle of the array)
///   4. Remove (remove chunks of data from the middle of an array)
pub fn main() !void {
    var buf: [256]u8 = undefined;

    // Open a fresh data file.
    Db.cleanup("sample1_crud") catch {};
    const db = try Db.open("sample1_crud");

    // Create a new variable. (Non-data query: element count is ignored.)
    _ = try db.execute(
        \\create example struct {
        \\  a f32,
        \\  b i32,
        \\  d [5][10] u32
        \\}
    , null);

    initExample(&src);

    // Insert data at offset 0. `{d}` (Zig fmt), not `%d` (printf).
    _ = try db.executeFmt(&buf, @ptrCast(&src), "insert example 0 {d}", .{200});

    // Read (most of) the data with a stride of 3.
    var n: usize = @intCast(try db.executeFmt(
        &buf,
        @ptrCast(&dest),
        "read example[0:-10:3] blimit {d}",
        .{@sizeOf(@TypeOf(dest))},
    ));
    printExample("Read elements: ", dest[0..n]);

    // Remove (most of) the data with a stride of 2.
    n = @intCast(try db.executeFmt(
        &buf,
        @ptrCast(&dest),
        "remove example[0:-10:2] blimit {d}",
        .{@sizeOf(@TypeOf(dest))},
    ));
    printExample("Removed elements: ", dest[0..n]);

    // Read all of the data.
    n = @intCast(try db.executeFmt(
        &buf,
        @ptrCast(&dest),
        "read example[0:] blimit {d}",
        .{@sizeOf(@TypeOf(dest))},
    ));
    printExample("After Remove: ", dest[0..n]);

    // Overwrite all of the data with src, then read it back.
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

fn initExample(ex: []Example) void {
    for (ex, 0..) |*e, i| {
        e.a = @floatFromInt(i);
        e.b = @intCast(i + 1);
        for (0..5) |r| {
            for (0..10) |col| {
                e.d[r][col] = @intCast(i + r * 10 + col);
            }
        }
    }
}
