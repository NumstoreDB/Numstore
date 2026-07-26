const std = @import("std");
const c = @cImport({
    @cInclude("smartfiles.h");
});

pub const Error = error{
    SmOpenFailed,
    Smfile,
};

pub const sb_size = i64; 
pub const b_size = u64;


pub const Smfile = struct {
    ptr: *c.smfile_t,    

    pub fn fromPtr(ptr: *c.smfile_t) Smfile {
        return .{ .ptr = ptr };
    } 

    pub fn open(path: [*:0]const u8) Error!Smfile {
        return .{ .ptr = c.smfile_open(path) orelse return error.SmOpenFailed };
    }

    pub fn cleanup(path: [*:0]const u8) Error!void {
        if (c.smfile_cleanup(path) < 0) return error.Smfile;
    }

    pub fn close(self: Smfile) Error!void {
        if (c.smfile_close(self.ptr) < 0) return error.Smfile;
    }

    pub fn crash(self: Smfile) Error!void {
        if (c.smfile_crash(self.ptr) < 0) return error.Smfile;
    }

    // Void src is supposed to be a generic type
    pub fn insert(self: Smfile, src: u8, bofst: sb_size, slen: b_size) Error!void {
        return if (c.smfile_insert(self.ptr, src, bofst, slen) < 0) return error.Smfile;
    }

};
