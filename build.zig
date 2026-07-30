const std = @import("std");
const fs = @import("src/build/filesystem.zig");
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

    //////////////////////////// Core
    const core_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .root_source_file = b.path("src/core/core.zig"),
    });
    const core_c = try fs.glob(b.allocator, b.graph.io, "src/core", ".c");
    core_module.addCSourceFiles(.{
        .root = b.path("src/core"),
        .files = core_c.items,
        .flags = &.{
            "-DNUMSTORE_LIB",
            "-DTESTING",
            "-DNLOG",
            "-fno-sanitize=alignment", // TODO - remove this
        },
    });
    core_module.addIncludePath(b.path("src/core"));

    //////////////////////////// NSCore
    const nscore_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .root_source_file = b.path("src/nscore/nscore.zig"),
        .imports = &.{
            .{ .name = "core", .module = core_module },
        },
    });
    const nscore_c = try fs.glob(b.allocator, b.graph.io, "src/nscore", ".c");
    core_module.addCSourceFiles(.{
        .root = b.path("src/nscore"),
        .files = nscore_c.items,
        .flags = &.{
            "-DNUMSTORE_LIB",
            "-DTESTING",
            "-DNLOG",
            "-fno-sanitize=alignment", // TODO - remove this
        },
    });
    core_module.addIncludePath(b.path("src"));

    //////////////////////////// Numstore
    const numstore_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .root_source_file = b.path("src/numstore/numstore.zig"),
        .imports = &.{
            .{ .name = "nscore", .module = nscore_module },
            .{ .name = "core", .module = core_module },
        },
    });
    const numstore_c = try fs.glob(b.allocator, b.graph.io, "src/numstore", ".c");
    numstore_module.addCSourceFiles(.{
        .root = b.path("src/numstore"),
        .files = numstore_c.items,
        .flags = &.{
            "-DNUMSTORE_LIB",
            "-DTESTING",
            "-DNLOG",
            "-fno-sanitize=alignment", // TODO - remove this
        },
    });
    numstore_module.addIncludePath(b.path("src"));

    //////////////////////////// Smartfiles
    const smartfiles_module = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .root_source_file = b.path("src/smartfiles/smartfiles.zig"),
        .imports = &.{
            .{ .name = "core", .module = core_module },
            .{ .name = "nscore", .module = nscore_module },
        },
    });
    const smartfiles_c = try fs.glob(b.allocator, b.graph.io, "src/smartfiles", ".c");
    smartfiles_module.addCSourceFiles(.{
        .root = b.path("src/smartfiles"),
        .files = smartfiles_c.items,
        .flags = &.{
            "-DNUMSTORE_LIB",
            "-DTESTING",
            "-DNLOG",
            "-fno-sanitize=alignment", // TODO - remove this
        },
    });
    smartfiles_module.addIncludePath(b.path("src"));

    //////////////////////////// Tests
    const test_module = b.createModule(.{
        .root_source_file = b.path("src/lib.zig"),
        .link_libc = true,
        .target = target,
        .imports = &.{
            .{ .name = "core", .module = core_module },
            .{ .name = "nscore", .module = nscore_module },
            .{ .name = "numstore", .module = numstore_module },
            .{ .name = "smartfiles", .module = smartfiles_module },
        },
    });

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
        .root_module = test_module,
    });

    const unit_tests = b.addRunArtifact(tests);

    test_step.dependOn(&unit_tests.step);
}
