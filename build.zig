const std = @import("std");
const gen_unit_tests = @import("src/build/gen_unit_tests.zig").gen_unit_tests;

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Generate unit_tests.c up front
    try gen_unit_tests(
        b.graph.io,
        b.allocator,
        &.{"src"},
        "src/templates/unit_tests.c.in",
        "src/unit_tests.c",
    );

    const c_srcs = .{
        "core/os_windows.c",
        "core/serial.c",
        "core/htable.c",
        "core/collections.c",
        "core/alloc.c",
        "core/numerics.c",
        "core/robin_hood_ht.c",
        "core/os_common.c",
        "core/utils.c",
        "core/testing.c",
        "core/error.c",
        "core/tests.c",
        "core/concurrency.c",
        "core/stride.c",
        "core/os_posix.c",
        "core/logging.c",
        "nscore/lock_table.c",
        "nscore/variables.c",
        "nscore/file_pager.c",
        "nscore/parsers.c",
        "nscore/wal.c",
        "nscore/dirty_page_table.c",
        "nscore/nsdb.c",
        "nscore/pager.c",
        "nscore/var_algorithms.c",
        "nscore/page.c",
        "nscore/types.c",
        "nscore/query.c",
        "nscore/compiler.c",
        "nscore/mem_vhmap.c",
        "nscore/txn_table.c",
        "nscore/page_fixture.c",
        "nscore/rope_algorithms.c",
        "nscore/node_updates.c",
        "numstore/numstore.c",
        "numstore/swarm_tests.c",
        "smartfiles/smfile_test_fixture.c",
        "smartfiles/smartfiles.c",
        "smartfiles/tests.c",
    };

    const numstore_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .root_source_file = b.path("src/lib.zig"),
    });
    numstore_module.addCSourceFiles(.{
        .root = b.path("src"),
        .files = &c_srcs,
        .flags = &.{
            "-DNUMSTORE_LIB",
            "-DTESTING",
            "-DNLOG",
            "-fno-sanitize=alignment", // TODO - remove this
        },
    });
    numstore_module.addIncludePath(b.path("src"));

    //////////////////////////// Build Numstore
    const numstore = b.addLibrary(.{
        .name = "numstore",
        .linkage = .dynamic,
        .root_module = numstore_module,
    });
    b.installArtifact(numstore);

    //////////////////////////// Build Tests
    const test_step = b.step("test", "Run unit tests");
    const tests = b.addTest(.{
        .name = "test",
        .root_module = numstore_module,
    });

    const unit_tests = b.addRunArtifact(tests);

    test_step.dependOn(&unit_tests.step);
}
