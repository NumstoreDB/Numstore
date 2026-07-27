pub const numstore = @import("numstore.zig");
const std = @import("std");

extern fn run_unit_tests(seed: c_int, filter: [*:0]const u8) c_int;

test "c_unit_tests" {
    const rc = run_unit_tests(1234, "my_filter");
    try std.testing.expectEqual(0, rc);
}
