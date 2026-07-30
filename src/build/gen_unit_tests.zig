const std = @import("std");
const Io = std.Io;
const assert = std.debug.assert;
const max_file_size = 8192 * 100;
const testing = std.testing;

/// Generates unit test .c file from a
/// template file. Searches through a directory
pub fn gen_unit_tests(
    io: Io,
    gpa: std.mem.Allocator,
    roots: []const []const u8,
    template_path: []const u8,
    dest_path: []const u8,
) !void {
    const cwd = Io.Dir.cwd();

    // Buffered Allocating file Writer
    var calls: Io.Writer.Allocating = .init(gpa);
    defer calls.deinit();

    const w = &calls.writer;
    var count: usize = 0;

    for (roots) |root| {
        var dir = try cwd.openDir(io, root, .{ .iterate = true });
        defer dir.close(io);

        // Iterate through all files in this directory
        var it = dir.iterate();

        while (try it.next(io)) |entry| {

            // Filter on files
            if (entry.kind != .file) {
                continue;
            }

            // Filter on ".c" files
            if (!std.mem.endsWith(u8, entry.name, ".c")) {
                continue;
            }

            // Read the entire file into memory
            const content = try dir.readFileAlloc(
                io,
                entry.name,
                gpa,
                .limited(max_file_size),
            );
            defer gpa.free(content);

            // Get the directory path
            var dir_path_buf: [std.fs.max_path_bytes]u8 = undefined;
            const dlen = try dir.realPath(io, &dir_path_buf);

            // The total path
            var path_buf: [std.fs.max_path_bytes]u8 = undefined;
            const path = try std.fmt.bufPrint(&path_buf, "{s}/{s}", .{ dir_path_buf[0..dlen], entry.name });

            try scanFile(w, &count, path, content);
        }
    }

    // Read in the template
    const template = try cwd.readFileAlloc(
        io,
        template_path,
        gpa,
        .limited(max_file_size),
    );
    defer gpa.free(template);

    // Print count into the template
    const count_str = try std.fmt.allocPrint(gpa, "{d}", .{count});
    defer gpa.free(count_str);

    // Replace CALLS with all calls
    const stage1 = try std.mem.replaceOwned(u8, gpa, template, "%CALLS%", calls.written());
    defer gpa.free(stage1);

    // Replace TEST_COUMT with count
    const final = try std.mem.replaceOwned(u8, gpa, stage1, "%TEST_COUNT%", count_str);
    defer gpa.free(final);

    // Create the output file
    const out = try cwd.createFile(io, dest_path, .{});
    defer out.close(io);

    // Write the entire test
    try out.writeStreamingAll(io, final);
}

fn isWord(c: u8) bool {
    return std.ascii.isAlphanumeric(c) or c == '_';
}

fn scanFile(w: *Io.Writer, count: *usize, path: []const u8, content: []const u8) !void {

    // Iterate through all instances of the string literal TEST
    var fofst: usize = 0;
    var num_tests: usize = 0;
    const max_tests_per_file: usize = 1024;

    while (num_tests < max_tests_per_file) : (num_tests += 1) {
        if (std.mem.indexOfPos(u8, content, fofst, "\nTEST (")) |start| {

            // \nTEST (func_name)
            //         ^
            //         7
            //
            // Skip all whitespace
            var j = start + 7;
            const name_start = j;

            // Expect word
            if (!(j < content.len and isWord(content[j]))) {
                fofst = start + 1;
                continue;
            }

            // Iterate over function name
            while (j < content.len and isWord(content[j])) {
                // Advance
                j += 1;
            }

            const name = content[name_start..j];
            assert(name.len > 0); // We checked that at least one value was a word

            // Find the line location of this string
            const line = std.mem.count(u8, content[0..start], "\n") + 1;
            try emitCall(w, path, line, name);
            count.* += 1;

            // Start at the previous value
            fofst = j;
        } else {
            // no tests
            break;
        }
    }
}

fn emitCall(w: *Io.Writer, path: []const u8, line: usize, name: []const u8) !void {
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
