const std = @import("std");
const builtin = @import("builtin");

const zon = @import("build.zig.zon");

comptime {
    const required = std.SemanticVersion{ .major = 0, .minor = 16, .patch = 0 };
    if (builtin.zig_version.order(required) == .lt) {
        @compileError("nvi requires Zig 0.16.0 or newer; found " ++ builtin.zig_version.string);
    }
}

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const mod = b.addModule("nvi", .{
        .root_source_file = b.path("src/root.zig"),
        .target = target,
    });

    const exe = b.addExecutable(.{
        .name = "nvi",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
            .imports = &.{
                .{ .name = "nvi", .module = mod },
            },
        }),
    });

    b.installArtifact(exe);

    // build_options carries the version and commit into the binary
    const options = b.addOptions();
    options.addOption([]const u8, "version", zon.version);
    options.addOption([]const u8, "commit", gitCommit(b));
    mod.addOptions("build_options", options);

    const run_step = b.step("run", "Run the app");
    const run_cmd = b.addRunArtifact(exe);
    run_step.dependOn(&run_cmd.step);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    // Top level test step depends on the unit and integration steps below, so
    // `zig build test` runs everything.
    const test_step = b.step("test", "Run all tests");

    // Unit tests: one test artifact per *_test.zig file so the summary reports
    // a separate pass count per file. Each file imports implementation modules
    // (some of which read build_options), so every test module gets it. Grouped
    // under their own step, runnable on their own with `zig build unit`.
    const unit_files = [_][]const u8{
        "accessors_test.zig",
        "arg_test.zig",
        "emitter_test.zig",
        "formatter_test.zig",
        "parser_test.zig",
        "scanner_test.zig",
        "tokenizer_test.zig",
        "utils_test.zig",
    };

    const unit_step = b.step("unit", "Run unit tests");
    test_step.dependOn(unit_step);

    for (unit_files) |file| {
        const test_mod = b.createModule(.{
            .root_source_file = b.path(b.fmt("src/{s}", .{file})),
            .target = target,
        });
        test_mod.addOptions("build_options", options);

        const unit_test = b.addTest(.{ .root_module = test_mod });
        const run_unit_test = b.addRunArtifact(unit_test);
        // Name by the module under test (strip the trailing "_test.zig").
        run_unit_test.setName(file[0 .. file.len - "_test.zig".len]);
        unit_step.dependOn(&run_unit_test.step);
    }

    // Integration tests run the real, compiled binary against committed .env
    // fixtures and assert behavior end to end. Grouped under their own step so
    // the summary nests them and so they can be run with `zig build integration`.
    // No cwd override is set, so each step runs from the build root and nvi
    // resolves the relative fixtures/ paths there. addStdoutCase asserts exact
    // stdout (and an implicit exit-code-0 check); addExitCase asserts an exit
    // code plus a stderr substring (a substring, so ANSI codes need not match).
    const integration_step = b.step("integration", "Run end-to-end tests against the compiled binary");
    test_step.dependOn(integration_step);

    addStdoutCase(b, exe, integration_step, "nul: emit pairs + command", &.{ "--files", "fixtures/test.env", "--", "printenv", "MESSAGE" }, "MESSAGE=hello\x00printenv\x00MESSAGE\x00");
    addStdoutCase(b, exe, integration_step, "nul: interpolation (prior key)", &.{ "--files", "fixtures/interp.env", "--", "true" }, "MESSAGE=hello\x00GREETING=hello world\x00true\x00");
    addStdoutCase(b, exe, integration_step, "nul: value with spaces", &.{ "--files", "fixtures/spaces.env", "--", "printenv", "SENTENCE" }, "SENTENCE=All your base\x00printenv\x00SENTENCE\x00");
    addStdoutCase(b, exe, integration_step, "powershell: format", &.{ "--format", "powershell", "--files", "fixtures/test.env", "--", "npm", "start" }, "$env:MESSAGE = 'hello'\n& 'npm' 'start'\n");
    addStdoutCase(b, exe, integration_step, "required: present", &.{ "--required", "API_KEY", "--files", "fixtures/required.env", "--", "true" }, "API_KEY=abc\x00true\x00");

    addExitCase(b, exe, integration_step, "required: missing (exit 1)", &.{ "--required", "NOPE", "--files", "fixtures/required.env", "--", "true" }, 1, "marked as required");
    addExitCase(b, exe, integration_step, "parse: unterminated interpolation (exit 1)", &.{ "--files", "fixtures/bad_interp.env", "--", "true" }, 1, "unterminated value interpolation");
    addExitCase(b, exe, integration_step, "parse: missing key name (exit 1)", &.{ "--files", "fixtures/empty_key.env", "--", "true" }, 1, "without a key name");
    addExitCase(b, exe, integration_step, "file: not found (exit 1)", &.{ "--files", "fixtures/nope.env", "--", "true" }, 1, "Unable to locate");
    addExitCase(b, exe, integration_step, "usage: missing command (exit 2)", &.{ "--files", "fixtures/test.env" }, 2, "must be defined and followed by a command");
    addExitCase(b, exe, integration_step, "usage: invalid format (exit 2)", &.{ "--format", "bogus", "--", "true" }, 2, "is not a valid format");
    addExitCase(b, exe, integration_step, "help (exit 0)", &.{"--help"}, 0, "Usage: nvi [flags]");
    addExitCase(b, exe, integration_step, "version (exit 0)", &.{"--version"}, 0, "nvi");
}

// Registers a Run step that launches the compiled binary with `args`, names it
// `name` (so it is identifiable in the build summary), attaches it to `step`,
// and asserts its stdout matches `expected_stdout` exactly. expectStdOutEqual
// also adds an implicit exit-code-0 check, so a clean run is verified too.
fn addStdoutCase(
    b: *std.Build,
    exe: *std.Build.Step.Compile,
    step: *std.Build.Step,
    name: []const u8,
    args: []const []const u8,
    expected_stdout: []const u8,
) void {
    const run = b.addRunArtifact(exe);
    run.addArgs(args);
    run.expectStdOutEqual(expected_stdout);
    run.setName(name);
    step.dependOn(&run.step);
}

// Registers a Run step that launches the compiled binary with `args`, names it
// `name`, attaches it to `step`, and asserts the process exits with `code` and
// that `stderr_match` appears somewhere in stderr (substring match, so
// surrounding ANSI color codes are ignored).
fn addExitCase(
    b: *std.Build,
    exe: *std.Build.Step.Compile,
    step: *std.Build.Step,
    name: []const u8,
    args: []const []const u8,
    code: u8,
    stderr_match: []const u8,
) void {
    const run = b.addRunArtifact(exe);
    run.addArgs(args);
    run.expectExitCode(code);
    run.addCheck(.{ .expect_stderr_match = stderr_match });
    run.setName(name);
    step.dependOn(&run.step);
}

// Best-effort short commit hash. Returns "" if git is missing, this isn't a
// repo, or the host can't spawn (e.g. some cross-compilation setups).
fn gitCommit(b: *std.Build) []const u8 {
    if (!std.process.can_spawn) return "";
    var code: u8 = undefined;
    const out = b.runAllowFail(
        &.{ "git", "-C", b.build_root.path orelse ".", "rev-parse", "--short", "HEAD" },
        &code,
        .ignore,
    ) catch return "";
    return std.mem.trimEnd(u8, out, "\n");
}
