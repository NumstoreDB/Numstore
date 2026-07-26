const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const csrcs = try glob(b.allocator, b.graph.io, "src", ".c");

    // Build Numstore
    const numstore = b.addExecutable(.{
        .name = "numstore",
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .root_source_file = b.path("src/main.zig"),
        }),
    });
    numstore.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .files = csrcs.items,
        .flags = &.{
            "-DNUMSTORE_LIB",
        },
    });
    numstore.root_module.addIncludePath(b.path("src"));


    // Build samples
    const sample1 = b.addExecutable(.{
        .name = "sample1",
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .root_source_file = b.path("src/samples/ns_sample1_basic_crud.zig"),
        }),
    });
    sample1.root_module.addImport("numstore", numstore.root_module);

    b.installArtifact(numstore);
    b.installArtifact(sample1);
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
