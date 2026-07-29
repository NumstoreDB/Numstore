const std = @import("std");
const fs = @import("src/build/filesystem.zig");
const gen_unit_tests = @import("src/build/gen_unit_tests.zig").gen_unit_tests;

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Generate unit_tests.c
    try gen_unit_tests(
        b.graph.io,
        b.allocator,
        &.{"src"},
        "src/templates/unit_tests.c.in",
        "src/unit_tests.c",
    );

    // C source code
    const csrcs = try fs.glob(b.allocator, b.graph.io, "src", ".c");

    //////////////////////////// Build Numstore
    const numstore = b.addLibrary(.{
        .name = "numstore",
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .root_source_file = b.path("src/lib.zig"),
        }),
    });
    numstore.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .files = csrcs.items,
        .flags = &.{
            "-DNUMSTORE_LIB",
            "-DTESTING",
            "-DNLOG",
            "-fno-sanitize=alignment", // TODO - remove this
        },
    });
    numstore.root_module.addIncludePath(b.path("src"));
    b.installArtifact(numstore);

    //////////////////////////// Build Tests
    const test_step = b.step("test", "Run unit tests");
    const tests = b.addTest(.{
        .name = "test",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/lib.zig"),
            .link_libc = true,
            .target = target,
        }),
    });
    tests.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .files = csrcs.items,
        .flags = &.{
            "-DNUMSTORE_LIB",
            "-DTESTING",
            "-DNLOG",
            "-fno-sanitize=alignment", // TODO - remove this
        },
    });
    tests.root_module.addIncludePath(b.path("src"));
    const unit_tests = b.addRunArtifact(tests);
    test_step.dependOn(&unit_tests.step);
}
