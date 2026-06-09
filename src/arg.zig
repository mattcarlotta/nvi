const std = @import("std");
const tty = @import("tty.zig");
const Io = std.Io;
const mem = std.mem;
const expect = std.testing.expect;

const IgnoredFlag = struct {
    flag: []const u8,
    params: std.ArrayList([]const u8) = .empty,
};

pub const Arg = struct {
    files: std.ArrayList([]const u8) = .empty,
    command: ?[]const [:0]const u8 = null,
    debug: bool = false,
    required: std.ArrayList([]const u8) = .empty,
    logger: *Io.Writer,

    const Flag = enum { files, debug, required };

    const flags = std.StaticStringMap(Flag).initComptime(.{
        .{ "-f", .files },
        .{ "--files", .files },
        .{ "-d", .debug },
        .{ "--debug", .debug },
        .{ "-r", .required },
        .{ "--required", .required },
    });

    pub fn validateFileName(self: *Arg, f: []const u8) !void {
        if (mem.indexOf(u8, f, ".env") == null) {
            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The file flag " ++ tty.bold ++ tty.red ++ "{s}" ++ tty.reset ++ " is not a valid env file (file must contain '.env' extension)\n", .{f});
            return error.InvalidFileExtension;
        }

        if (std.fs.path.isAbsolute(f)) {
            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The file flag " ++ tty.bold ++ tty.red ++ "{s}" ++ tty.reset ++ " must be relative to the current directory\n", .{f});
            return error.InvalidFilePath;
        }

        var it = mem.tokenizeScalar(u8, f, '/');
        while (it.next()) |component| {
            if (mem.eql(u8, component, "..")) {
                try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The file flag " ++ tty.bold ++ tty.red ++ "{s}" ++ tty.reset ++ " may not escape the current directory\n", .{f});
                return error.InvalidFilePathEscape;
            }
        }
    }

    pub fn printFlags(self: *Arg) !void {
        try self.logger.writeAll(tty.cyan ++ "info: " ++ tty.reset ++ "The following flags have been set... " ++ tty.cyan ++ "\n• command: " ++ tty.reset);
        if (self.command) |cmd| {
            for (cmd, 0..) |part, idx| {
                if (idx != 0) try self.logger.writeByte(' ');
                try self.logger.writeAll(part);
            }
        } else {
            try self.logger.writeAll("(none)");
        }

        try self.logger.writeAll(tty.cyan ++ "\n• files: " ++ tty.reset);
        for (self.files.items, 0..) |f, i| {
            if (i != 0) try self.logger.writeAll(", ");
            try self.logger.writeAll(f);
        }

        try self.logger.writeAll(tty.cyan ++ "\n• required: " ++ tty.reset);
        if (self.required.items.len > 0) {
            for (self.required.items, 0..) |f, i| {
                if (i != 0) try self.logger.writeAll(", ");
                try self.logger.writeAll(f);
            }
        } else {
            try self.logger.writeAll("(none)");
        }

        try self.logger.writeByte('\n');
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

pub fn argParser(alloc: mem.Allocator, argv: []const [:0]const u8, logger: *Io.Writer) !Arg {
    var args: Arg = .{ .logger = logger };
    var iter: ArgvIter = .{ .argv = argv };

    var ignored_flags: std.ArrayList(IgnoredFlag) = .empty;
    defer {
        for (ignored_flags.items) |*entry| entry.params.deinit(alloc);
        ignored_flags.deinit(alloc);
    }

    // skipping program name at argv[0]
    _ = iter.next();

    while (iter.next()) |token| {
        if (mem.eql(u8, token, "--")) {
            args.command = iter.rest();
            break;
        }

        const flag = Arg.flags.get(token) orelse {
            var entry: IgnoredFlag = .{ .flag = token };

            while (iter.nextValue()) |param| try entry.params.append(alloc, param);

            try ignored_flags.append(alloc, entry);

            continue;
        };

        switch (flag) {
            .debug => args.debug = true,
            .files => {
                const first = iter.nextValue() orelse {
                    try args.logger.writeAll(tty.red ++ "error:" ++ tty.reset ++ " Flag " ++ tty.bold ++ tty.red ++ "{s}" ++ tty.reset ++ " requires at least 1 parameter\n");
                    return error.MissingValue;
                };

                try args.validateFileName(first);

                try args.files.append(alloc, first);

                while (iter.nextValue()) |file| {
                    try args.validateFileName(file);
                    try args.files.append(alloc, file);
                }
            },
            .required => {
                const first = iter.nextValue() orelse {
                    try args.logger.writeAll(tty.red ++ "error:" ++ tty.reset ++ " Flag " ++ tty.bold ++ tty.red ++ "{s}" ++ tty.reset ++ " requires at least 1 parameter\n");
                    return error.MissingValue;
                };

                try args.required.append(alloc, first);

                while (iter.nextValue()) |file| try args.required.append(alloc, file);
            },
        }
    }

    for (ignored_flags.items) |entry| {
        try args.logger.print(
            tty.yellow ++ "warning: " ++ tty.reset ++ "An unrecognized flag " ++ tty.bold ++ tty.yellow ++ "{s}" ++ tty.reset,
            .{entry.flag},
        );

        if (entry.params.items.len > 0) {
            try args.logger.writeAll(" with parameters " ++ tty.bold ++ tty.yellow);

            for (entry.params.items, 0..) |p, idx| {
                if (idx != 0) try args.logger.writeByte(' ');
                try args.logger.writeAll(p);
            }
        }

        try args.logger.writeAll(tty.reset ++ " was ignored \n");
    }

    if (args.files.items.len == 0) {
        try args.files.append(alloc, ".env");
    }

    if (args.command == null or args.command.?.len == 0) {
        try args.logger.writeAll(tty.red ++ "error:" ++ tty.reset ++ " An " ++ tty.italic ++ "end of options delimiter" ++ tty.reset ++ " (--) must be defined and followed by a command (e.g., nvi <flags>" ++ tty.green ++ " -- <command>" ++ tty.reset ++ ")\n");
        return error.MissingCommand;
    }

    if (args.debug) {
        try args.printFlags();
    }

    return args;
}

test "parses and sets debug flag" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    var argv = [_][:0]const u8{ "nvi", "--debug", "--", "echo", "hi" };
    var logger_buf: [1024]u8 = undefined;
    var logger: Io.Writer = .fixed(&logger_buf);

    const args = try argParser(alloc, &argv, &logger);

    try expect(args.debug);
}

test "parses and sets files flag" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    var argv = [_][:0]const u8{ "nvi", "--files", ".env.local", ".env", "--", "echo", "hi" };
    var logger_buf: [1024]u8 = undefined;
    var logger: Io.Writer = .fixed(&logger_buf);

    const args = try argParser(alloc, &argv, &logger);

    try expect(args.files.items.len == 2);
    try std.testing.expectEqualStrings(".env.local", args.files.items[0]);
    try std.testing.expectEqualStrings(".env", args.files.items[1]);
}

test "errors on file flag missing .env extension" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    var argv = [_][:0]const u8{ "nvi", "--files", "example", "--", "echo", "hi" };
    var logger_buf: [1024]u8 = undefined;
    var logger: Io.Writer = .fixed(&logger_buf);

    _ = argParser(alloc, &argv, &logger) catch {};

    try expect(mem.indexOf(u8, logger.buffered(), "is not a valid env file") != null);
}

test "errors on a relative file flag" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    var argv = [_][:0]const u8{ "nvi", "--files", "/home/.env", "--", "echo", "hi" };
    var logger_buf: [1024]u8 = undefined;
    var logger: Io.Writer = .fixed(&logger_buf);

    _ = argParser(alloc, &argv, &logger) catch {};

    try expect(mem.indexOf(u8, logger.buffered(), "must be relative to the current directory") != null);
}

test "errors on a escape file flag" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    var argv = [_][:0]const u8{ "nvi", "--files", "../.env", "--", "echo", "hi" };
    var logger_buf: [1024]u8 = undefined;
    var logger: Io.Writer = .fixed(&logger_buf);

    _ = argParser(alloc, &argv, &logger) catch {};

    try expect(mem.indexOf(u8, logger.buffered(), "may not escape the current directory") != null);
}

test "parses and sets required envs flag" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    var argv = [_][:0]const u8{ "nvi", "--required", "ENV_1", "ENV_2", "--", "echo", "hi" };
    var logger_buf: [1024]u8 = undefined;
    var logger: Io.Writer = .fixed(&logger_buf);

    const args = try argParser(alloc, &argv, &logger);

    try expect(args.required.items.len == 2);
    try std.testing.expectEqualStrings("ENV_1", args.required.items[0]);
    try std.testing.expectEqualStrings("ENV_2", args.required.items[1]);
}

test "warns on unknown flag" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    var argv = [_][:0]const u8{ "nvi", "--unknown", "x", "--", "echo", "hi" };
    var logger_buf: [1024]u8 = undefined;
    var logger: Io.Writer = .fixed(&logger_buf);

    _ = try argParser(alloc, &argv, &logger);

    try expect(mem.indexOf(u8, logger.buffered(), "unrecognized flag") != null);
}
