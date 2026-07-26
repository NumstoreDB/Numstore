const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const csrcs = try glob(b.allocator, b.graph.io, "src", ".c");

    // Build the numstore module
    const cmodule = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .root_source_file = b.path("src/main.zig"),
        }); 
    cmodule.addCSourceFiles(.{
        .root = b.path("src"),
        .files = csrcs.items,
        .flags = &.{
            "-DNUMSTORE_LIB",
        },
    });
    cmodule.addIncludePath(b.path("src"));

    const numstore = b.addExecutable(.{
        .name = "numstore",
        .linkage = .dynamic,
        .root_module = cmodule,
    });

    b.installArtifact(numstore);
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
