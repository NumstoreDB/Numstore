const builtin = @import("builtin");
const std = @import("std");
const panic = std.debug.panic;
const assert = std.debug.assert;

pub const test_helpers = if (builtin.is_test) struct {
    pub fn fileOrDirExists(io: std.Io, fname: []const u8) !bool {
        const cwd = std.Io.Dir.cwd();
        _ = cwd.statFile(io, fname, .{}) catch |err| switch (err) {
            error.FileNotFound => return false,
            else => return err,
        };
        return true;
    }

    const FileContent = struct {
        name: []const u8,
        content: []const u8,
    };

    pub fn makeFiles(io: std.Io, dir: []const u8, files: []FileContent) void {
        const cwd = std.Io.Dir.cwd();
        var path_buf: [std.fs.max_path_bytes]u8 = undefined;

        // Ensure dir doesn't exist
        const dir_exists = fileOrDirExists(io, dir) catch |err| {
            panic("Failed to check dir existence: {t}", .{err});
        };
        if (dir_exists) {
            panic("Refusing to overwrite dir: {s} just for testing\n", .{dir});
        }

        // Create the directory
        cwd.createDir(io, dir, .default_dir) catch |err| {
            panic("Failed to create dir: {t}", .{err});
        };

        // Create files
        for (0..files.len) |i| {
            const path = std.fmt.bufPrint(&path_buf, "{s}/{s}", .{ dir, files[i].name }) catch |err| {
                panic("Path too long: {s}/{s}: {t}", .{ dir, files[i].name, err });
            };

            // All files don't exist because we validated this was a new directory
            const file_exists = fileOrDirExists(io, path) catch |err| {
                panic("Failed to check file existence: {t}", .{err});
            };
            assert(!file_exists);

            writeOneFile(io, cwd, path, files[i].content) catch |err| {
                panic("Failed writing file {s}: {t}", .{ path, err });
            };
        }
    }

    fn writeOneFile(io: std.Io, cwd: std.Io.Dir, path: []const u8, content: []const u8) !void {
        var file = try cwd.createFile(io, path, .{});
        errdefer {
            cwd.deleteFile(io, path) catch {
                std.debug.print("Failed to delete file: {s}\n", .{path});
            };
        }
        defer file.close(io);
        try file.writeAll(io, content);
    }
} else struct {};
