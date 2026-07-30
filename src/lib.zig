pub const numstore = @import("numstore");
pub const smartfiles = @import("smartfiles");

const c = @cImport({
    @cDefine("TESTING", "1");
    @cInclude("unit_tests.h");
    @cInclude("numstore/swarm_tests.h");
});
const std = @import("std");

// TODO - make these optional in build.zig
// test "c unit tests" {
//     const ret = c.run_unit_tests(1234, "");
//     try std.testing.expectEqual(0, ret);
// }
//
// test "c irwr tests" {
//     const ret = c.irwr_swarm_test("irwr", 10, 1234);
//     try std.testing.expectEqual(0, ret);
// }
//
// test "c cgd tests" {
//     const ret = c.cgd_swarm_test("cgd", 10, 1234);
//     try std.testing.expectEqual(0, ret);
// }

test {
    @import("std").testing.refAllDecls(@This());
}
