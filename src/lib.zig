pub const numstore = @import("numstore.zig");
pub const smartfiles = @import("smartfiles.zig");

const c = @cImport({
    @cDefine("TESTING", "1");
    @cInclude("unit_tests.h");
});
const std = @import("std");

// test "c unit tests" {
//     const ret = c.run_unit_tests(1234, "");
//     try std.testing.expectEqual(0, ret);
// }

test {
    @import("std").testing.refAllDecls(@This());
}
