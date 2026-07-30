const std = @import("std");

const stdtypes = @import("stdtypes.zig");
const sb_size = stdtypes.sb_size;
const b_size = stdtypes.b_size;
const t_size = stdtypes.t_size;

pub const Error = error{
    Io,
    NoMem,
    Arith,
    Corrupt,
    InvalidArgument,
    PgOutOfRange,
    Syntax,
    Interp,
    RptreePageStackOverflow,
    DuplicateVariable,
    VariableNe,
    DuplicateCommit,
};

pub fn fromInt(v: i32) Error!void {
    return switch (v) {
        -1 => error.Io,
        -2 => error.NoMem,
        -3 => error.Arith,
        -4 => error.Corrupt,
        -5 => error.InvalidArgument,
        -6 => error.PgOutOfRange,
        -7 => error.Syntax,
        -8 => error.Interp,
        -9 => error.RptreePageStackOverflow,
        -10 => error.DuplicateVariable,
        -11 => error.VariableNe,
        -12 => error.DuplicateCommit,
        else => unreachable,
    };
}

pub fn check_int(v: c_int) Error!void {
    if (v < 0) {
        try fromInt(v);
        unreachable;
    }
}

pub fn check_sb_size(v: sb_size) Error!b_size {
    if (v < 0) {
        try fromInt(@intCast(v));
        unreachable;
    }
    return @intCast(v);
}
