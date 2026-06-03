const std = @import("std");

const cyan = "\x1b[36m";
const reset = "\x1b[0m";

pub const Arg = struct {
    files: std.ArrayList([]const u8) = .empty,
    command: ?[]const [:0]const u8 = null,
    debug: bool = false,
    err: ?Error = null,

    const Flag = enum { files, debug };

    const flags = std.StaticStringMap(Flag).initComptime(.{
        .{ "-f", .files },
        .{ "--files", .files },
        .{ "-dbg", .debug },
        .{ "--debug", .debug },
    });

    pub const Error = union(enum) {
        missing_value: []const u8,
        missing_command,

        pub fn print(self: Error) void {
            switch (self) {
                .missing_value => |flag| std.log.err("flag '{s}' requires a value", .{flag}),
                .missing_command => std.log.err("a '--' (end of options delimiter) must be defined and followed by a command to run (e.g., nvi -- echo 'Hello')", .{}),
            }
        }
    };

    pub fn debugPrint(self: Arg) !void {
        var buf: [4096]u8 = undefined;
        var w: std.Io.Writer = .fixed(&buf);

        try w.writeAll("parsed flags...");
        try w.writeAll(cyan);
        try w.writeAll("\nfiles: ");
        try w.writeAll(reset);
        if (self.files.items.len > 0) {
            for (self.files.items, 0..) |f, i| {
                if (i != 0) try w.writeAll(", ");
                try w.writeAll(f);
                try w.writeAll(" (default)");
            }
        } else {
            try w.writeAll("(none)");
        }

        try w.writeAll(cyan);
        try w.writeAll("\ncommand: ");
        try w.writeAll(reset);
        if (self.command) |cmd| {
            if (cmd.len > 0) {
                for (cmd, 0..) |part, idx| {
                    if (idx != 0) try w.writeByte(' ');
                    try w.writeAll(part);
                }
            } else {
                try w.writeAll("(none)");
            }
        }

        std.log.info("{s}", .{w.buffered()});
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
        if (std.mem.startsWith(u8, nxt, "-")) return null;
        return self.next();
    }

    fn rest(self: *ArgvIter) []const [:0]const u8 {
        return self.argv[self.i..];
    }
};

pub fn argParser(alloc: std.mem.Allocator, argv: []const [:0]const u8) !Arg {
    var args: Arg = .{};
    var iter: ArgvIter = .{ .argv = argv };

    // skipping program name at argv[0]
    _ = iter.next();

    while (iter.next()) |token| {
        if (std.mem.eql(u8, token, "--")) {
            args.command = iter.rest();
            break;
        }

        const flag = Arg.flags.get(token) orelse {
            std.log.warn("ignoring unrecognized argument: {s}", .{token});
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
        }
    }

    if (args.files.items.len == 0) {
        try args.files.append(alloc, ".env");
    }

    if (args.command == null or args.command.?.len == 0) {
        args.err = .missing_command;
    }

    if (args.debug) {
        args.debugPrint() catch {};
    }

    return args;
}
