const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const csrcs = try glob(b.allocator, b.graph.io, "src", ".c");

    // Build Numstore
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
            "-DNLOG",
            "-fno-sanitize=alignment", // TODO - remove this
        },
    });
    numstore.root_module.addIncludePath(b.path("src"));

    // Build samples
    const ns_sample1 = b.addExecutable(.{
        .name = "ns_sample1",
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .root_source_file = b.path("src/samples/ns_sample1_basic.zig"),
            .imports = &.{
                .{ .name = "numstore", .module = numstore.root_module },
            },
        }),
    });
    ns_sample1.root_module.linkLibrary(numstore);

    // Build samples
    const ns_sample2 = b.addExecutable(.{
        .name = "ns_sample2",
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .root_source_file = b.path("src/samples/ns_sample2_struct.zig"),
            .imports = &.{
                .{ .name = "numstore", .module = numstore.root_module },
            },
        }),
    });
    ns_sample2.root_module.linkLibrary(numstore);

    b.installArtifact(numstore);
    b.installArtifact(ns_sample1);
    b.installArtifact(ns_sample2);
}

pub fn glob(
    allocator: std.mem.Allocator,
    io: std.Io,
    path: []const u8,
    suffix: []const u8,
) !std.ArrayList([]const u8) {
    var dir = try std.Io.Dir.cwd().openDir(io, path, .{ .iterate = true });
    defer dir.close(io);

    var it = dir.iterate();

    var results: std.ArrayList([]const u8) = .empty;
    errdefer {
        for (results.items) |p| allocator.free(p);
        results.deinit(allocator);
    }

    while (try it.next(io)) |entry| {
        if (entry.kind == .file and std.mem.endsWith(u8, entry.name, suffix)) {
            try results.append(allocator, try allocator.dupe(u8, entry.name));
        }
    }
    return results;
}
