const std = @import("std");
const Io = std.Io;

pub fn gen_unit_tests(
    io: Io,
    gpa: std.mem.Allocator,
    roots: []const []const u8,
    template_path: []const u8,
    dest_path: []const u8,
) !void {
    const cwd = Io.Dir.cwd();

    var calls: Io.Writer.Allocating = .init(gpa);
    defer calls.deinit();
    const w = &calls.writer;

    var count: usize = 0;

    for (roots) |root| {
        var dir = try cwd.openDir(io, root, .{ .iterate = true });
        defer dir.close(io);

        var walker = try dir.walk(gpa);
        defer walker.deinit();

        while (try walker.next(io)) |entry| {
            if (entry.kind != .file) continue;
            if (!std.mem.endsWith(u8, entry.basename, ".c")) continue;

            const content = entry.dir.readFileAlloc(io, entry.basename, gpa, .unlimited) catch continue;
            defer gpa.free(content);

            try scanFile(w, &count, entry.path, content);
        }
    }

    const template = try cwd.readFileAlloc(io, template_path, gpa, .unlimited);
    defer gpa.free(template);

    const count_str = try std.fmt.allocPrint(gpa, "{d}", .{count});
    defer gpa.free(count_str);

    const stage1 = try std.mem.replaceOwned(u8, gpa, template, "%CALLS%", calls.written());
    defer gpa.free(stage1);
    const final = try std.mem.replaceOwned(u8, gpa, stage1, "%TEST_COUNT%", count_str);
    defer gpa.free(final);

    const out = try cwd.createFile(io, dest_path, .{});
    defer out.close(io);
    try out.writeStreamingAll(io, final);
}

fn isWord(c: u8) bool {
    return std.ascii.isAlphanumeric(c) or c == '_';
}

/// Mirrors TEST\s*\(\s*(\w+)\s*\), scanning left-to-right. No I/O, so no `io` needed.
fn scanFile(w: *Io.Writer, count: *usize, path: []const u8, content: []const u8) !void {
    var i: usize = 0;
    while (std.mem.indexOfPos(u8, content, i, "TEST")) |start| {
        var j = start + 4;
        while (j < content.len and std.ascii.isWhitespace(content[j])) j += 1;
        if (j >= content.len or content[j] != '(') {
            i = start + 1;
            continue;
        }
        j += 1;
        while (j < content.len and std.ascii.isWhitespace(content[j])) j += 1;

        const name_start = j;
        while (j < content.len and isWord(content[j])) j += 1;
        const name = content[name_start..j];
        if (name.len == 0) {
            i = start + 1;
            continue;
        }

        while (j < content.len and std.ascii.isWhitespace(content[j])) j += 1;
        if (j >= content.len or content[j] != ')') {
            i = start + 1;
            continue;
        }
        j += 1;

        const line = std.mem.count(u8, content[0..start], "\n") + 1;
        try emitCall(w, path, line, name);
        count.* += 1;
        i = j;
    }
}

fn emitCall(w: *Io.Writer, path: []const u8, line: usize, name: []const u8) !void {
    // {{ }} are literal braces; \\n emits a literal backslash-n into the C source.
    try w.print(
        \\  //////////////////// {s}:{d} START
        \\  if (!filter || strstr("{s}", filter))
        \\  {{
        \\    extern void __test__{s}(void);
        \\    i_log_info("========================= TEST CASE: %s\n", "{s}");
        \\    int prev = test_ret;
        \\    test_ret = 0;
        \\    __test__{s}();
        \\    if (!test_ret)
        \\    {{
        \\      i_log_passed("%s\n", "{s}");
        \\      test_ret = prev;
        \\    }}
        \\    else
        \\    {{
        \\      failed_names[failed++] = "{s}";
        \\    }}
        \\    ntests++;
        \\  }}
        \\  //////////////////// {s}:{d} DONE
        \\
    , .{ path, line, name, name, name, name, name, name, path, line });
}
