const std = @import("std");

/// Glob all files flat inside path [path]
/// with suffixes in [suffix]
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
