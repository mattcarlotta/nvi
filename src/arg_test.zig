const std = @import("std");
const Io = std.Io;
const mem = std.mem;
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;
const expectError = std.testing.expectError;

const arg = @import("arg.zig");
const fmt = @import("formatter.zig");

const TestArgs = struct {
    arena: std.heap.ArenaAllocator,
    logger_buf: [4096]u8 = undefined,
    logger: Io.Writer = undefined,

    fn init() TestArgs {
        return .{ .arena = std.heap.ArenaAllocator.init(std.testing.allocator) };
    }

    fn deinit(self: *TestArgs) void {
        self.arena.deinit();
    }

    fn output(self: *TestArgs) []const u8 {
        return self.logger.buffered();
    }

    fn run(self: *TestArgs, argv: []const [:0]const u8) !arg.Arg {
        self.logger = .fixed(&self.logger_buf);
        var args: arg.Arg = .{ .alloc = self.arena.allocator(), .argv = argv, .logger = &self.logger };

        try args.run();

        return args;
    }
};

test "parses and sets dry-run flag" {
    var t = TestArgs.init();
    defer t.deinit();

    const args = try t.run(&.{ "nvi", "--dry-run", "--", "echo", "hi" });

    try expect(args.dry_run);
}

test "parses and sets files flag" {
    var t = TestArgs.init();
    defer t.deinit();

    const args = try t.run(&.{ "nvi", "--files", ".env.local", ".env", "--", "echo", "hi" });

    try expect(args.files.items.len == 2);
    try expectEqualStrings(".env.local", args.files.items[0]);
    try expectEqualStrings(".env", args.files.items[1]);
}

test "errors on file flag missing .env extension" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.InvalidFileExtension, t.run(&.{ "nvi", "--files", "example", "--", "echo", "hi" }));
    try expect(mem.indexOf(u8, t.output(), "is not a valid env file") != null);
}

test "errors on an absolute file flag" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.InvalidFilePath, t.run(&.{ "nvi", "--files", "/home/.env", "--", "echo", "hi" }));
    try expect(mem.indexOf(u8, t.output(), "must be relative to the current directory") != null);
}

test "errors on an escape file flag" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.InvalidFilePathEscape, t.run(&.{ "nvi", "--files", "../.env", "--", "echo", "hi" }));
    try expect(mem.indexOf(u8, t.output(), "may not escape the current directory") != null);
}

test "parses and sets required envs flag" {
    var t = TestArgs.init();
    defer t.deinit();

    const args = try t.run(&.{ "nvi", "--required", "ENV_1", "ENV_2", "--", "echo", "hi" });

    try expect(args.required.items.len == 2);
    try expectEqualStrings("ENV_1", args.required.items[0]);
    try expectEqualStrings("ENV_2", args.required.items[1]);
}

test "parses and sets format flag" {
    var t = TestArgs.init();
    defer t.deinit();

    const args = try t.run(&.{ "nvi", "--format", "powershell", "--", "echo", "hi" });

    try expectEqual(fmt.Format.powershell, args.format);
}

test "errors on an invalid format parameter" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.InvalidFormat, t.run(&.{ "nvi", "--format", "json", "--", "echo", "hi" }));
    try expect(mem.indexOf(u8, t.output(), "is not a valid format") != null);
}

test "warns on unknown flag" {
    var t = TestArgs.init();
    defer t.deinit();

    _ = try t.run(&.{ "nvi", "--unknown", "x", "--", "echo", "hi" });

    try expect(mem.indexOf(u8, t.output(), "unrecognized flag") != null);
}

test "errors on missing file flag parameter" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.MissingValue, t.run(&.{ "nvi", "--files", "--", "echo", "hi" }));
    try expect(mem.indexOf(u8, t.output(), "--files") != null);
    try expect(mem.indexOf(u8, t.output(), "requires at least 1 parameter") != null);
}

test "help prints usage to stderr and short-circuits" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.Help, t.run(&.{ "nvi", "help" }));
    try expect(mem.indexOf(u8, t.output(), "Usage: nvi [flags] -- <command>") != null);
}

test "version prints to stderr and short-circuits" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.Version, t.run(&.{ "nvi", "version" }));
    try expect(mem.indexOf(u8, t.output(), "nvi") != null);
    try expect(mem.indexOf(u8, t.output(), "commit") != null);
    try expect(mem.indexOf(u8, t.output(), "zig") != null);
}

test "help after the delimiter is a command token" {
    var t = TestArgs.init();
    defer t.deinit();

    const args = try t.run(&.{ "nvi", "--", "help" });
    try expectEqualStrings("help", args.command[0]);
}

test "parses and normalizes scan extensions" {
    var t = TestArgs.init();
    defer t.deinit();

    const args = try t.run(&.{ "nvi", "scan", "*.zig", ".ts", "mjs" });

    try expectEqual(@as(usize, 3), args.scan.items.len);
    try expectEqualStrings("zig", args.scan.items[0]);
    try expectEqualStrings("ts", args.scan.items[1]);
    try expectEqualStrings("mjs", args.scan.items[2]);
}

test "scan does not require a command" {
    var t = TestArgs.init();
    defer t.deinit();

    _ = try t.run(&.{ "nvi", "scan", "zig" });
}

test "errors on a shell-expanded filename as a scan extension" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.InvalidExtension, t.run(&.{ "nvi", "scan", "index.mjs" }));
    try expect(mem.indexOf(u8, t.output(), "shell may have expanded it") != null);
}

test "errors on missing scan extension parameter" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.MissingValue, t.run(&.{ "nvi", "scan" }));
    try expect(mem.indexOf(u8, t.output(), "scan") != null);
    try expect(mem.indexOf(u8, t.output(), "requires at least 1 parameter") != null);
}

test "parses and sets ignore flag" {
    var t = TestArgs.init();
    defer t.deinit();

    const args = try t.run(&.{ "nvi", "scan", "mjs", "--ignored", "NODE_ENV", "--", "echo", "hi" });

    try expectEqual(@as(usize, 1), args.ignored.items.len);
    try expectEqualStrings("NODE_ENV", args.ignored.items[0]);
}
