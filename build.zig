const std = @import("std");
const zcc = @import("zig_compile_commands");

pub fn build(b: *std.Build) !void {
    const build_samples = b.option(
        bool,
        "samples",
        "Build the sample executables",
    ) orelse false;

    // A list of targets for zcc
    var targets: std.ArrayList(*std.Build.Step.Compile) = .empty;

    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Gather all c source files for the library
    const csrcs = try glob(b.allocator, b.graph.io, "src", ".c");

    // Build Numstore
    const numstore = b.addLibrary(.{
        .name = "numstore",
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .root_source_file = b.path("src/lib.zig"),
        }),
    });
    numstore.root_module.addCSourceFiles(.{
        .root = b.path("src"),
        .files = csrcs.items,
        .flags = &.{
            "-DNUMSTORE_LIB",
            "-DNLOG",
            "-fno-sanitize=alignment", // TODO - remove this
        },
    });
    numstore.root_module.addIncludePath(b.path("src"));
    b.installArtifact(numstore);
    try targets.append(b.allocator, numstore);

    if (build_samples) {
        // Build Zig Samples
        try build_zig_sample(b, &targets, "ns_zig_sample1", b.path("src/samples/ns_sample1_basic.zig"), target, optimize, numstore);

        try build_zig_sample(b, &targets, "ns_zig_sample2", b.path("src/samples/ns_sample2_struct.zig"), target, optimize, numstore);

        try build_zig_sample(b, &targets, "ns_zig_sample3", b.path("src/samples/ns_sample3_transactions.zig"), target, optimize, numstore);

        // Build c Samples
        try build_c_sample(b, &targets, "ns_c_sample1", b.path("src/samples/ns_sample1_basic_crud.c"), target, optimize, numstore);

        try build_c_sample(b, &targets, "smfile_c_sample1", b.path("src/samples/smfile_sample1_basic_crud.c"), target, optimize, numstore);

        try build_c_sample(b, &targets, "smfile_c_sample2", b.path("src/samples/smfile_sample2_transactions.c"), target, optimize, numstore);

        try build_c_sample(b, &targets, "smfile_c_sample3", b.path("src/samples/smfile_sample3_stride.c"), target, optimize, numstore);

        try build_c_sample(b, &targets, "smfile_c_sample4", b.path("src/samples/smfile_sample4_rollback_commit.c"), target, optimize, numstore);
    }

    // Add compile_commands.json for c family language servers
    _ = zcc.createStep(b, "cdb", try targets.toOwnedSlice(b.allocator));
}

pub fn build_zig_sample(
    b: *std.Build,
    targets: *std.ArrayList(*std.Build.Step.Compile),
    name: []const u8,
    path: std.Build.LazyPath,
    target: ?std.Build.ResolvedTarget,
    optimize: ?std.builtin.OptimizeMode,
    numstore: *std.Build.Step.Compile,
) !void {
    const sample = b.addExecutable(.{
        .name = name,
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .root_source_file = path,
            .imports = &.{
                .{ .name = "numstore", .module = numstore.root_module },
            },
        }),
    });
    sample.root_module.linkLibrary(numstore);
    b.installArtifact(sample);
    try targets.append(b.allocator, sample);
}

pub fn build_c_sample(
    b: *std.Build,
    targets: *std.ArrayList(*std.Build.Step.Compile),
    name: []const u8,
    path: std.Build.LazyPath,
    target: ?std.Build.ResolvedTarget,
    optimize: ?std.builtin.OptimizeMode,
    numstore: *std.Build.Step.Compile,
) !void {
    //
    const sample = b.addExecutable(.{
        .name = name,
        .linkage = .dynamic,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    sample.root_module.addCSourceFile(.{
        .file = path,
        .flags = &.{},
    });
    sample.root_module.linkLibrary(numstore);
    sample.root_module.addIncludePath(b.path("src"));
    b.installArtifact(sample);
    try targets.append(b.allocator, sample);
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
