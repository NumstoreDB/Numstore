const builtin = @import("builtin");
const std = @import("std");

pub const test_helpers = if (builtin.is_test) struct {
    pub fn fileExists(io: std.Io, fname: []const u8) !bool {
        const file = std.Io.Dir.cwd().openFile(io, fname, .{}) catch |err| switch (err) {
            error.FileNotFound => {
                return false;
            },
            else => return err,
        };
        defer file.close(io);
        return true;
    }
} else struct {};
