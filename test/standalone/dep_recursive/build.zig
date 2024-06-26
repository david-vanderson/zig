const std = @import("std");

pub fn build(b: *std.Build) void {
    const test_step = b.step("test", "Test it");
    b.default_step = test_step;

    const optimize: std.builtin.OptimizeMode = .Debug;

    const foo = b.createModule(.{
        .root_source_file = b.path("foo.zig"),
    });
    foo.addImport("foo", foo);

    const exe = b.addExecutable(.{
        .name = "test",
        .root_source_file = b.path("test.zig"),
        .target = b.host,
        .optimize = optimize,
    });
    exe.root_module.addImport("foo", foo);

    const run = b.addRunArtifact(exe);
    test_step.dependOn(&run.step);
}
