const std = @import("std");
const Io = std.Io;
const expect = std.testing.expect;

const bold = "\x1b[1m";
const italic = "\x1b[3m";

const red = "\x1b[31m";
const green = "\x1b[32m";
const yellow = "\x1b[33m";
const cyan = "\x1b[36m";
const reset = "\x1b[0m";

const Ignored = struct {
    flag: []const u8,
    params: std.ArrayList([]const u8) = .empty,
};

pub const Arg = struct {
    files: std.ArrayList([]const u8) = .empty,
    command: ?[]const [:0]const u8 = null,
    debug: bool = false,
    required: std.ArrayList([]const u8) = .empty,
    err: ?Error = null,
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

    pub const Error = union(enum) {
        missing_value: []const u8,
        missing_command,

        pub fn print(self: Error, logger: *Io.Writer) !void {
            switch (self) {
                .missing_value => |flag| {
                    try logger.print(red ++ "error" ++ reset ++ ": Flag " ++ bold ++ red ++ "{s}" ++ reset ++ " requires at least 1 parameter\n", .{flag});
                },
                .missing_command => {
                    try logger.writeAll(red ++ "error" ++ reset ++ ": An " ++ italic ++ "end of options delimiter" ++ reset ++ " (--) must be defined and followed by a command to run (e.g., nvi <flags>" ++ green ++ " -- <command>" ++ reset ++ ")\n");
                },
            }
        }
    };

    pub fn debugPrint(self: Arg) !void {
        try self.logger.writeAll(cyan ++ "info: " ++ reset ++ "The following flags have been set... " ++ cyan ++ "\n• command: " ++ reset);
        if (self.command) |cmd| {
            for (cmd, 0..) |part, idx| {
                if (idx != 0) try self.logger.writeByte(' ');
                try self.logger.writeAll(part);
            }
        }

        try self.logger.writeAll(cyan ++ "\n• files: " ++ reset);
        for (self.files.items, 0..) |f, i| {
            if (i != 0) try self.logger.writeAll(", ");
            try self.logger.writeAll(f);
        }

        try self.logger.writeAll(cyan ++ "\n• required: " ++ reset);
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
        return if (std.mem.startsWith(u8, nxt, "-")) null else self.next();
    }

    fn rest(self: *ArgvIter) []const [:0]const u8 {
        return self.argv[self.i..];
    }
};

pub fn argParser(alloc: std.mem.Allocator, argv: []const [:0]const u8, logger: *Io.Writer) !Arg {
    var args: Arg = .{ .logger = logger };
    var iter: ArgvIter = .{ .argv = argv };
    var ignored: std.ArrayList(Ignored) = .empty;
    defer {
        for (ignored.items) |*entry| entry.params.deinit(alloc);
        ignored.deinit(alloc);
    }

    // skipping program name at argv[0]
    _ = iter.next();

    while (iter.next()) |token| {
        if (std.mem.eql(u8, token, "--")) {
            args.command = iter.rest();
            break;
        }

        const flag = Arg.flags.get(token) orelse {
            var entry: Ignored = .{ .flag = token };

            while (iter.nextValue()) |param| try entry.params.append(alloc, param);

            try ignored.append(alloc, entry);

            continue;
        };

        switch (flag) {
            .debug => {
                args.debug = true;
            },
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

    if (ignored.items.len > 0) {
        for (ignored.items) |entry| {
            try args.logger.print(
                yellow ++ "warning: " ++ reset ++ "An unrecognized flag " ++ bold ++ yellow ++ "{s}" ++ reset,
                .{entry.flag},
            );

            if (entry.params.items.len > 0) {
                try args.logger.writeAll(" with parameters " ++ bold ++ yellow);

                for (entry.params.items, 0..) |p, idx| {
                    if (idx != 0) try logger.writeByte(' ');
                    try args.logger.writeAll(p);
                }
            }

            try args.logger.writeAll(reset ++ " was ignored \n");
        }
    }

    if (args.command == null or args.command.?.len == 0) {
        args.err = .missing_command;
        return args;
    }

    if (args.files.items.len == 0) {
        try args.files.append(alloc, ".env");
    }

    if (args.debug) {
        try args.debugPrint();
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
