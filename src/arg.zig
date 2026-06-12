const std = @import("std");
const tty = @import("tty.zig");
const fm = @import("formatter.zig");

const Io = std.Io;
const mem = std.mem;
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;
const expectError = std.testing.expectError;

const version = @import("build_options").version;

const IgnoredFlag = struct {
    flag: []const u8,
    params: []const [:0]const u8 = &.{},
};

pub const Arg = struct {
    files: std.ArrayList([]const u8) = .empty,
    command: []const [:0]const u8 = &.{},
    debug: bool = false,
    format: fm.Format = .default,
    required: std.ArrayList([]const u8) = .empty,
    ignored: std.ArrayList([]const u8) = .empty,
    scan: std.ArrayList([]const u8) = .empty,
    logger: *Io.Writer,

    const Flag = enum { files, debug, format, ignored, required, scan, help, version };

    const flags = std.StaticStringMap(Flag).initComptime(.{
        .{ "-f", .files },
        .{ "--files", .files },
        .{ "-d", .debug },
        .{ "--debug", .debug },
        .{ "-i", .ignored },
        .{ "--ignored", .ignored },
        .{ "-F", .format },
        .{ "--format", .format },
        .{ "-r", .required },
        .{ "--required", .required },
        .{ "scan", .scan },
        .{ "help", .help },
        .{ "version", .version },
    });

    pub fn validateFileName(self: *Arg, f: []const u8) !void {
        if (mem.indexOf(u8, f, ".env") == null) {
            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The file flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " is not a valid env file (file must contain '.env' extension)\n", .{f});
            return error.InvalidFileExtension;
        }

        if (std.fs.path.isAbsolute(f)) {
            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The file flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " must be relative to the current directory\n", .{f});
            return error.InvalidFilePath;
        }

        var it = mem.tokenizeScalar(u8, f, '/');
        while (it.next()) |component| {
            if (mem.eql(u8, component, "..")) {
                try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The file flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " may not escape the current directory\n", .{f});
                return error.InvalidFilePathEscape;
            }
        }
    }

    fn parseExt(self: *Arg, e: []const u8) ![]const u8 {
        var ext = e;
        if (mem.startsWith(u8, ext, "*.")) ext = ext[2..];
        if (mem.startsWith(u8, ext, ".")) ext = ext[1..];

        if (!isAlphanumeric(ext)) {
            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The scan parameter " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " is not a file extension (e.g. mjs or *.mjs). If you passed an unquoted glob, your shell may have expanded it into filenames; pass the bare extension instead\n", .{e});
            return error.InvalidExtension;
        }

        return ext;
    }

    pub fn printFlags(self: *Arg) !void {
        try self.logger.writeAll(tty.cyan ++ "info: " ++ tty.reset ++ "The following flags have been set... \n");
        try self.logger.writeAll("   command: ");
        if (self.command.len > 0) {
            for (self.command, 0..) |part, idx| {
                if (idx != 0) try self.logger.writeByte(' ');
                try self.logger.print(tty.bold_green ++ "{s}" ++ tty.reset, .{part});
            }
        } else {
            try self.logger.writeAll("(undefined)");
        }

        try self.logger.writeAll("\n   files: ");
        for (self.files.items, 0..) |f, i| {
            if (i != 0) try self.logger.writeAll(", ");
            try self.logger.print(tty.bold_green ++ "{s}" ++ tty.reset, .{f});
        }

        try self.logger.writeAll("\n   required: ");
        if (self.required.items.len > 0) {
            for (self.required.items, 0..) |f, i| {
                if (i != 0) try self.logger.writeAll(", ");
                try self.logger.print(tty.bold_green ++ "{s}" ++ tty.reset, .{f});
            }
        } else {
            try self.logger.writeAll("(undefined)");
        }

        try self.logger.writeAll("\n   scan: ");
        if (self.scan.items.len > 0) {
            for (self.scan.items, 0..) |f, i| {
                if (i != 0) try self.logger.writeAll(", ");
                try self.logger.print(tty.bold_green ++ "{s}" ++ tty.reset, .{f});
            }
        } else {
            try self.logger.writeAll("(undefined)");
        }

        try self.logger.print("\n   format: " ++ tty.bold_green ++ "{t}" ++ tty.reset, .{self.format});

        try self.logger.writeAll("\n\n");
    }
};

fn isAlphanumeric(s: []const u8) bool {
    if (s.len == 0) return false;
    for (s) |c| {
        if (!std.ascii.isAlphanumeric(c)) return false;
    }
    return true;
}

pub fn printHelp(out: *Io.Writer) !void {
    try out.writeAll(
        \\Usage: nvi [flags] -- <command>
        \\       nvi scan [ext]
        \\
        \\Flags:
        \\  -f, --files <paths>     parses .env files in order (default: .env)
        \\  -i, --ignored <keys>    ignores ENV keys that scan should not add to the required ENV key list
        \\  -r, --required <keys>   marks a list of ENV keys that must be present after parsing
        \\  -F, --format <fmt>      outputs ENV format (options: nul|powershell)
        \\  -d, --debug             prints internal debug details to stderr
        \\
        \\Commands:
        \\  help                    prints this help and exits
        \\  scan <ext>              recursively scans in the root directory for ENV keys used within *.<ext> files and marks them as required
        \\  version                 prints the version and exits
        \\
        \\Example:
        \\  nvi --files .env -- cargo run | xargs -0 env
        \\  nvi scan *.ts --ignored NODE_ENV -- npm run dev | xargs -0 env
        \\
    );
}

const ArgvIter = struct {
    argv: []const [:0]const u8,
    i: usize = 0,

    fn next(self: *ArgvIter) ?[:0]const u8 {
        if (self.i >= self.argv.len) return null;
        defer self.i += 1;

        return self.argv[self.i];
    }

    fn peek(self: *ArgvIter) ?[:0]const u8 {
        return if (self.i < self.argv.len) self.argv[self.i] else null;
    }

    fn nextValue(self: *ArgvIter) ?[:0]const u8 {
        const nxt = self.peek() orelse return null;

        return if (mem.startsWith(u8, nxt, "-")) null else self.next();
    }

    fn requireValue(self: *ArgvIter, args: *Arg, token: []const u8) ![:0]const u8 {
        return self.nextValue() orelse {
            try args.logger.print(tty.red ++ "error:" ++ tty.reset ++ " Flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " requires at least 1 parameter\n", .{token});
            return error.MissingValue;
        };
    }

    fn rest(self: *ArgvIter) []const [:0]const u8 {
        return self.argv[self.i..];
    }
};

pub fn argParser(alloc: mem.Allocator, argv: []const [:0]const u8, logger: *Io.Writer) !Arg {
    var args: Arg = .{ .logger = logger };
    var iter: ArgvIter = .{ .argv = argv };

    var ignored_flags: std.ArrayList(IgnoredFlag) = .empty;
    defer ignored_flags.deinit(alloc);

    // skipping program name at argv[0]
    _ = iter.next();

    while (iter.next()) |token| {
        if (mem.eql(u8, token, "--")) {
            args.command = iter.rest();
            break;
        }

        const flag = Arg.flags.get(token) orelse {
            const start = iter.i;
            while (iter.nextValue()) |_| {}
            try ignored_flags.append(alloc, .{ .flag = token, .params = iter.argv[start..iter.i] });

            continue;
        };

        switch (flag) {
            .debug => args.debug = true,
            .help => {
                try printHelp(args.logger);
                return error.Help;
            },
            .ignored => {
                var param = try iter.requireValue(&args, token);
                while (true) {
                    try args.ignored.append(alloc, param);
                    param = iter.nextValue() orelse break;
                }
            },
            .files => {
                var param = try iter.requireValue(&args, token);
                while (true) {
                    try args.validateFileName(param);
                    try args.files.append(alloc, param);
                    param = iter.nextValue() orelse break;
                }
            },
            .format => {
                const param = try iter.requireValue(&args, token);

                args.format = std.meta.stringToEnum(fm.Format, param) orelse {
                    try args.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The format flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " is not a valid format (expected 'nul' or 'powershell')\n", .{param});
                    return error.InvalidFormat;
                };
            },
            .required => {
                var param = try iter.requireValue(&args, token);
                while (true) {
                    try args.required.append(alloc, param);
                    param = iter.nextValue() orelse break;
                }
            },
            .scan => {
                var param = try iter.requireValue(&args, token);
                while (true) {
                    try args.scan.append(alloc, try args.parseExt(param));
                    param = iter.nextValue() orelse break;
                }
            },
            .version => {
                try args.logger.print("nvi {s}\n", .{version});
                return error.Version;
            },
        }
    }

    for (ignored_flags.items) |entry| {
        try args.logger.print(
            tty.yellow ++ "warning: " ++ tty.reset ++ "An unrecognized flag " ++ tty.bold_yellow ++ "{s}" ++ tty.reset,
            .{entry.flag},
        );

        if (entry.params.len > 0) {
            try args.logger.writeAll(" with parameters " ++ tty.bold_yellow);

            for (entry.params, 0..) |p, idx| {
                if (idx != 0) try args.logger.writeByte(' ');
                try args.logger.writeAll(p);
            }
        }

        try args.logger.writeAll(tty.reset ++ " was ignored \n");
    }

    if (args.files.items.len == 0) {
        try args.files.append(alloc, ".env");
    }

    if (args.scan.items.len == 0 and args.command.len == 0) {
        try args.logger.writeAll(tty.red ++ "error:" ++ tty.reset ++ " An " ++ tty.italic ++ "end of options delimiter" ++ tty.reset ++ " (--) must be defined and followed by a command (e.g., nvi <flags>" ++ tty.green ++ " -- <command>" ++ tty.reset ++ "). See nvi help\n");
        return error.MissingCommand;
    }

    if (args.debug) {
        try args.printFlags();
    }

    return args;
}

const TestArgs = struct {
    arena: std.heap.ArenaAllocator,
    logger_buf: [2048]u8 = undefined,
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

    fn run(self: *TestArgs, argv: []const [:0]const u8) !Arg {
        self.logger = .fixed(&self.logger_buf);
        return argParser(self.arena.allocator(), argv, &self.logger);
    }
};

test "parses and sets debug flag" {
    var t = TestArgs.init();
    defer t.deinit();

    const args = try t.run(&.{ "nvi", "--debug", "--", "echo", "hi" });

    try expect(args.debug);
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

    try expectEqual(fm.Format.powershell, args.format);
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
    try expect(mem.indexOf(u8, t.output(), "nvi ") != null);
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
