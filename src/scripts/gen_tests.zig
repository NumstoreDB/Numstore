const std = @import("std");

const MAX_FILE = 50 * 1024 * 1024;

pub const Test = struct {
    name: []const u8,
    filepath: []const u8,
    line: usize,
};

pub const Error = error{
    DirNotFound,
    NoTests,
} || std.mem.Allocator.Error || std.Io.Dir.OpenError;

// ---------------------------------------------------------------------------
// find_tests(roots) -> []Test
// Walks each root for *.c files (sorted), scans them for `TEST(name)`,
// and returns the collected tests. Input: roots. Output: owned slice.
// ---------------------------------------------------------------------------
pub fn findTests(io: std.Io, allocator: std.mem.Allocator, roots: []const []const u8) ![]Test {
    var tests: std.ArrayListUnmanaged(Test) = .empty;
    errdefer tests.deinit(allocator);

    for (roots) |root| {
        std.debug.print("{d}\n", .{root});
        var dir = std.Io.Dir.cwd().openDir(io, root, .{ .iterate = true }) catch |err| switch (err) {
            error.FileNotFound, error.NotDir => return Error.DirNotFound,
            else => return err,
        };
        defer dir.close(io);

        var files: std.ArrayListUnmanaged([]const u8) = .empty;
        defer {
            for (files.items) |f| allocator.free(f);
            files.deinit(allocator);
        }

        var walker = try dir.walk(allocator);
        defer walker.deinit();
        while (try walker.next(io)) |entry| {
            if (entry.kind != .file) continue;
            if (!std.mem.endsWith(u8, entry.basename, ".c")) continue;
            try files.append(allocator, try allocator.dupe(u8, entry.path));
        }
        std.mem.sort([]const u8, files.items, {}, lessThanStr);

        for (files.items) |subpath| {
            const content = dir.readFileAlloc(io, subpath, allocator, std.Io.Limit.limited(1000)) catch continue;
            defer allocator.free(content);

            const full = try std.fs.path.join(allocator, &.{ root, subpath });
            defer allocator.free(full);

            try scanContent(allocator, content, full, &tests);
        }
    }

    return tests.toOwnedSlice(allocator);
}

fn lessThanStr(_: void, a: []const u8, b: []const u8) bool {
    return std.mem.lessThan(u8, a, b);
}

// Hand-rolled equivalent of the regex  TEST\s*\(\s*(\w+)\s*\)
fn scanContent(
    allocator: std.mem.Allocator,
    content: []const u8,
    filepath: []const u8,
    tests: *std.ArrayListUnmanaged(Test),
) !void {
    var i: usize = 0;
    while (std.mem.indexOfPos(u8, content, i, "TEST")) |pos| {
        var j = pos + 4;
        j = skipWs(content, j);
        if (j >= content.len or content[j] != '(') {
            i = pos + 4;
            continue;
        }
        j = skipWs(content, j + 1);

        const name_start = j;
        while (j < content.len and isWord(content[j])) j += 1;
        const name_end = j;
        if (name_start == name_end) {
            i = pos + 4;
            continue;
        }

        j = skipWs(content, j);
        if (j >= content.len or content[j] != ')') {
            i = pos + 4;
            continue;
        }

        const line = std.mem.count(u8, content[0..pos], "\n") + 1;
        try tests.append(allocator, .{
            .name = try allocator.dupe(u8, content[name_start..name_end]),
            .filepath = try allocator.dupe(u8, filepath),
            .line = line,
        });
        i = j + 1; // non-overlapping, like re.finditer
    }
}

fn skipWs(s: []const u8, start: usize) usize {
    var k = start;
    while (k < s.len and std.ascii.isWhitespace(s[k])) k += 1;
    return k;
}

fn isWord(c: u8) bool {
    return std.ascii.isAlphanumeric(c) or c == '_';
}

// ---------------------------------------------------------------------------
// make_calls(tests) -> string
// Input: the tests. Output: the generated C dispatch code.
// ---------------------------------------------------------------------------
pub fn makeCalls(allocator: std.mem.Allocator, tests: []const Test) ![]u8 {
    var out: std.ArrayListUnmanaged(u8) = .empty;
    errdefer out.deinit(allocator);

    for (tests) |t| {
        const block = try std.fmt.allocPrint(allocator,
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
        , .{
            t.filepath, t.line,
            t.name,     t.name,
            t.name,     t.name,
            t.name,     t.name,
            t.filepath, t.line,
        });
        defer allocator.free(block);
        try out.appendSlice(allocator, block);
    }

    return out.toOwnedSlice(allocator);
}

// ---------------------------------------------------------------------------
// generate(tests, template) -> output string
// Input: tests + template text. Output: the filled-in text (NOT written).
// ---------------------------------------------------------------------------
pub fn generate(allocator: std.mem.Allocator, tests: []const Test, template: []const u8) ![]u8 {
    const calls = try makeCalls(allocator, tests);
    defer allocator.free(calls);

    const with_calls = try std.mem.replaceOwned(u8, allocator, template, "%CALLS%", calls);
    defer allocator.free(with_calls);

    const count = try std.fmt.allocPrint(allocator, "{d}", .{tests.len});
    defer allocator.free(count);

    return std.mem.replaceOwned(u8, allocator, with_calls, "%TEST_COUNT%", count);
}

// ---------------------------------------------------------------------------
// Reads template, runs generate, writes output. `io` is passed in, not made.
// ---------------------------------------------------------------------------
pub fn gen_tests(
    io: std.Io,
    a: std.mem.Allocator,
    template_path: []const u8,
    output_path: []const u8,
    search_dirs: []const []const u8,
) !void {
    const tests = try findTests(io, a, search_dirs);
    if (tests.len == 0) return Error.NoTests;

    const template = try std.Io.Dir.cwd().readFileAlloc(io, template_path, a, std.Io.Limit.limited(1000));
    const output = try generate(a, tests, template);
    try std.Io.Dir.cwd().writeFile(io, .{ .sub_path = output_path, .data = output });
}
