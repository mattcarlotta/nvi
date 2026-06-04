const tty = @import("tty.zig");
const std = @import("std");
const Io = std.Io;
const expect = std.testing.expect;

const IgnoredFlag = struct {
    flag: []const u8,
    params: std.ArrayList([]const u8) = .empty,
};

const Arg = struct {
    files: std.ArrayList([]const u8) = .empty,
    command: ?[]const [:0]const u8 = null,
    debug: bool = false,
    required: std.ArrayList([]const u8) = .empty,
    err: ?Error = null,

    const Flag = enum { files, debug, required };

    const flags = std.StaticStringMap(Flag).initComptime(.{
        .{ "-f", .files },
        .{ "--files", .files },
        .{ "-d", .debug },
        .{ "--debug", .debug },
        .{ "-r", .required },
        .{ "--required", .required },
    });

    pub const Error = union(enum) {
        missing_value: []const u8,
        missing_command,

        pub fn getMessage(self: Error, alloc: std.mem.Allocator) ![]u8 {
            return switch (self) {
                .missing_value => |flag| std.fmt.allocPrint(
                    alloc,
                    tty.red ++ "error" ++ tty.reset ++ ": Flag " ++ tty.bold ++ tty.red ++ "{s}" ++ tty.reset ++ " requires at least 1 parameter\n",
                    .{flag},
                ),
                .missing_command => std.fmt.allocPrint(
                    alloc,
                    tty.red ++ "error" ++ tty.reset ++ ": An " ++ tty.italic ++ "end of options delimiter" ++ tty.reset ++ " (--) must be defined and followed by a command (e.g., nvi <flags>" ++ tty.green ++ " -- <command>" ++ tty.reset ++ ")\n",
                    .{},
                ),
            };
        }
    };

    pub fn printFlags(self: Arg, logger: *Io.Writer) !void {
        try logger.writeAll(tty.cyan ++ "info: " ++ tty.reset ++ "The following flags have been set... " ++ tty.cyan ++ "\n• command: " ++ tty.reset);
        if (self.command) |cmd| {
            for (cmd, 0..) |part, idx| {
                if (idx != 0) try logger.writeByte(' ');
                try logger.writeAll(part);
            }
        } else {
            try logger.writeAll("(none)");
        }

        try logger.writeAll(tty.cyan ++ "\n• files: " ++ tty.reset);
        for (self.files.items, 0..) |f, i| {
            if (i != 0) try logger.writeAll(", ");
            try logger.writeAll(f);
        }

        try logger.writeAll(tty.cyan ++ "\n• required: " ++ tty.reset);
        if (self.required.items.len > 0) {
            for (self.required.items, 0..) |f, i| {
                if (i != 0) try logger.writeAll(", ");
                try logger.writeAll(f);
            }
        } else {
            try logger.writeAll("(none)");
        }

        try logger.writeByte('\n');
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

        return if (std.mem.startsWith(u8, nxt, "-")) null else self.next();
    }

    fn rest(self: *ArgvIter) []const [:0]const u8 {
        return self.argv[self.i..];
    }
};

pub fn argParser(alloc: std.mem.Allocator, argv: []const [:0]const u8, logger: *Io.Writer) !Arg {
    var args: Arg = .{};
    var iter: ArgvIter = .{ .argv = argv };

    var ignored_flags: std.ArrayList(IgnoredFlag) = .empty;
    defer {
        for (ignored_flags.items) |*entry| entry.params.deinit(alloc);
        ignored_flags.deinit(alloc);
    }

    // skipping program name at argv[0]
    _ = iter.next();

    while (iter.next()) |token| {
        if (std.mem.eql(u8, token, "--")) {
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
                    args.err = .{ .missing_value = token };
                    return args;
                };

                try args.files.append(alloc, first);

                while (iter.nextValue()) |file| try args.files.append(alloc, file);
            },
            .required => {
                const first = iter.nextValue() orelse {
                    args.err = .{ .missing_value = token };
                    return args;
                };

                try args.required.append(alloc, first);

                while (iter.nextValue()) |file| try args.required.append(alloc, file);
            },
        }
    }

    for (ignored_flags.items) |entry| {
        try logger.print(
            tty.yellow ++ "warning: " ++ tty.reset ++ "An unrecognized flag " ++ tty.bold ++ tty.yellow ++ "{s}" ++ tty.reset,
            .{entry.flag},
        );

        if (entry.params.items.len > 0) {
            try logger.writeAll(" with parameters " ++ tty.bold ++ tty.yellow);

            for (entry.params.items, 0..) |p, idx| {
                if (idx != 0) try logger.writeByte(' ');
                try logger.writeAll(p);
            }
        }

        try logger.writeAll(tty.reset ++ " was ignored \n");
    }

    if (args.files.items.len == 0) {
        try args.files.append(alloc, ".env");
    }

    if (args.command == null or args.command.?.len == 0) {
        args.err = .missing_command;
        return args;
    }

    if (args.debug) {
        try args.printFlags(logger);
    }

    return args;
}

test "warns on unknown flag" {
    var argv = [_][:0]const u8{ "nvi", "--unknown", "x", "--", "echo", "hi" };
    var logger_buf: [1024]u8 = undefined;
    var logger_w: Io.Writer = .fixed(&logger_buf);

    var args = try argParser(std.testing.allocator, &argv, &logger_w);
    defer args.files.deinit(std.testing.allocator);

    try expect(std.mem.indexOf(u8, logger_w.buffered(), "--unknown") != null);
}
