const std = @import("std");
const tty = @import("tty.zig");
const fmt = @import("formatter.zig");
const version = @import("build_options").version;

const Io = std.Io;
const mem = std.mem;
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectError = std.testing.expectError;
const expectEqualStrings = std.testing.expectEqualStrings;

const IgnoredFlag = struct {
    flag: []const u8,
    params: []const [:0]const u8 = &.{},
};

pub const Arg = struct {
    command: []const [:0]const u8 = &.{},
    debug: bool = false,
    files: std.ArrayList([]const u8) = .empty,
    format: fmt.Format = .default,
    required: std.ArrayList([]const u8) = .empty,
    logger: *Io.Writer,

    const Flag = enum { debug, files, format, help, required, version };

    const flags = std.StaticStringMap(Flag).initComptime(.{ .{ "help", .help }, .{ "-f", .files }, .{ "--files", .files }, .{ "-F", .format }, .{ "--format", .format }, .{ "-d", .debug }, .{ "--debug", .debug }, .{ "-r", .required }, .{ "--required", .required }, .{ "version", .version } });

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

    pub fn print(self: *Arg) !void {
        try self.logger.writeAll(tty.cyan ++ "info: " ++ tty.reset ++ "The following flags have been set... \n");
        try self.logger.writeAll("   command: ");
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

        try self.logger.print("\n   format: " ++ tty.bold_green ++ "{t}" ++ tty.reset, .{self.format});

        try self.logger.writeAll("\n   required: ");
        if (self.required.items.len > 0) {
            for (self.required.items, 0..) |f, i| {
                if (i != 0) try self.logger.writeAll(", ");
                try self.logger.print(tty.bold_green ++ "{s}" ++ tty.reset, .{f});
            }
        } else {
            try self.logger.writeAll("(none)");
        }

        try self.logger.writeAll("\n\n");
    }
};

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

    fn rest(self: *ArgvIter) []const [:0]const u8 {
        return self.argv[self.i..];
    }
};

pub fn argParser(alloc: mem.Allocator, argv: []const [:0]const u8, stdout: *Io.Writer, logger: *Io.Writer) !Arg {
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
            .help => {
                try stdout.writeAll(
                    \\Usage: nvi [flags] -- <command>
                    \\
                    \\Flags:
                    \\  -f, --files <paths...>     .env files to parse, in order and space delimited (default: .env)
                    \\  -r, --required <keys...>   keys that must be present after parsing (default: undefined)
                    \\  -F, --format <fmt>         output format: nul or powershell (default: nul)
                    \\  -d, --debug                print parsing details to stderr (default: false)
                    \\
                    \\Options:
                    \\  help                       print this help and exit
                    \\  version                    print the binary version and exit
                    \\
                    \\Example:
                    \\  nvi --files .env -- npm run dev | xargs -0 env
                    \\
                );
                try stdout.writeByte('\n');
                return error.Help;
            },
            .debug => args.debug = true,
            .files => {
                const first = iter.nextValue() orelse {
                    try args.logger.print(tty.red ++ "error:" ++ tty.reset ++ " Flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " requires at least 1 parameter\n", .{token});
                    return error.MissingValue;
                };

                try args.validateFileName(first);

                try args.files.append(alloc, first);

                while (iter.nextValue()) |file| {
                    try args.validateFileName(file);
                    try args.files.append(alloc, file);
                }
            },
            .format => {
                const param = iter.nextValue() orelse {
                    try args.logger.print(tty.red ++ "error:" ++ tty.reset ++ " Flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " requires a parameter (nul or powershell)\n", .{token});
                    return error.MissingValue;
                };

                args.format = std.meta.stringToEnum(fmt.Format, param) orelse {
                    try args.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The format flag " ++ tty.bold_red ++ "{s}" ++ tty.reset, .{param});
                    try args.logger.writeAll(" is not a valid format. Expected " ++ tty.bold_green ++ "nul" ++ tty.reset ++ " or " ++ tty.bold_green ++ "powershell" ++ tty.reset ++ ".\n");
                    return error.InvalidFormat;
                };
            },
            .required => {
                const first = iter.nextValue() orelse {
                    try args.logger.print(tty.red ++ "error:" ++ tty.reset ++ " Flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " requires at least 1 parameter\n", .{token});
                    return error.MissingValue;
                };

                try args.required.append(alloc, first);

                while (iter.nextValue()) |file| try args.required.append(alloc, file);
            },
            .version => {
                try stdout.print("v{s}\n", .{version});
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

    if (args.command.len == 0) {
        try args.logger.writeAll(tty.red ++ "error:" ++ tty.reset ++ " An " ++ tty.italic ++ "end of options delimiter" ++ tty.reset ++ " (--) must be defined and followed by a command (e.g., nvi <flags>" ++ tty.green ++ " -- <command>" ++ tty.reset ++ ")\n");
        return error.MissingCommand;
    }

    if (args.debug) {
        try args.print();
    }

    return args;
}

const TestArgs = struct {
    arena: std.heap.ArenaAllocator,
    logger_buf: [1024]u8 = undefined,
    logger: Io.Writer = undefined,
    stdout_buf: [1024]u8 = undefined,
    stdout: Io.Writer = undefined,

    fn init() TestArgs {
        return .{ .arena = std.heap.ArenaAllocator.init(std.testing.allocator) };
    }

    fn deinit(self: *TestArgs) void {
        self.arena.deinit();
    }

    fn stdoutOutput(self: *TestArgs) []const u8 {
        return self.stdout.buffered();
    }

    fn output(self: *TestArgs) []const u8 {
        return self.logger.buffered();
    }

    fn run(self: *TestArgs, argv: []const [:0]const u8) !Arg {
        self.stdout = .fixed(&self.stdout_buf);
        self.logger = .fixed(&self.logger_buf);
        return argParser(self.arena.allocator(), argv, &self.stdout, &self.logger);
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

test "parses and sets format flag" {
    var t = TestArgs.init();
    defer t.deinit();

    const args = try t.run(&.{ "nvi", "--format", "powershell", "--", "echo", "hi" });

    try expectEqual(fmt.Format.powershell, args.format);
}

test "defaults format to the platform default" {
    var t = TestArgs.init();
    defer t.deinit();

    const args = try t.run(&.{ "nvi", "--", "echo", "hi" });

    try expectEqual(fmt.Format.default, args.format);
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

test "errors on an invalid format parameter" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.InvalidFormat, t.run(&.{ "nvi", "--format", "json", "--", "echo", "hi" }));
    try expect(mem.indexOf(u8, t.output(), "is not a valid format") != null);
}

test "errors on missing format parameter" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.MissingValue, t.run(&.{ "nvi", "--format", "--", "echo", "hi" }));
    try expect(mem.indexOf(u8, t.output(), "--format") != null);
}

test "help prints usage to stdout and short-circuits" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.Help, t.run(&.{ "nvi", "help" }));
    try expect(mem.indexOf(u8, t.stdoutOutput(), "Usage: nvi [flags] -- <command>") != null);
}

test "version prints to stdout and short-circuits" {
    var t = TestArgs.init();
    defer t.deinit();

    try expectError(error.Version, t.run(&.{ "nvi", "version" }));
    try expect(mem.indexOf(u8, t.stdoutOutput(), "v") != null);
}
