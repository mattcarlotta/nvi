const std = @import("std");
const tty = @import("tty.zig");
const fmt = @import("formatter.zig");
const utils = @import("utils.zig");
const char = @import("char.zig");
const version = @import("build_options").version;

const Io = std.Io;
const mem = std.mem;
const path = std.fs.path;

const IgnoredFlag = struct {
    flag: []const u8,
    params: []const [:0]const u8 = &.{},
};

pub const Arg = struct {
    alloc: mem.Allocator,
    argv: []const [:0]const u8,
    i: usize = 0,
    files: std.ArrayList([]const u8) = .empty,
    command: []const [:0]const u8 = &.{},
    debug: bool = false,
    format: fmt.Format = .default,
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

        .{ "-h", .help },
        .{ "--help", .help },

        .{ "-i", .ignored },
        .{ "--ignored", .ignored },

        .{ "-F", .format },
        .{ "--format", .format },

        .{ "-r", .required },
        .{ "--required", .required },

        .{ "-s", .scan },
        .{ "--scan", .scan },

        .{ "-v", .version },
        .{ "--version", .version },

        .{ "scan", .scan },
        .{ "help", .help },
        .{ "version", .version },
    });

    pub fn run(self: *Arg) !void {
        var invalid_flags: std.ArrayList(IgnoredFlag) = .empty;
        defer invalid_flags.deinit(self.alloc);

        // skipping program name at argv[0]
        _ = self.next();

        while (self.next()) |token| {
            if (mem.eql(u8, token, "--")) {
                self.command = self.rest();
                break;
            }

            const flag = Arg.flags.get(token) orelse {
                const start = self.i;
                while (self.nextValue()) |_| {}
                try invalid_flags.append(self.alloc, .{ .flag = token, .params = self.argv[start..self.i] });

                continue;
            };

            switch (flag) {
                .debug => self.debug = true,
                .help => {
                    try utils.printHelp(self.logger);
                    return error.Help;
                },
                .ignored => {
                    var param = try self.requireValue(token);
                    while (true) {
                        try self.ignored.append(self.alloc, param);
                        param = self.nextValue() orelse break;
                    }
                },
                .files => {
                    var param = try self.requireValue(token);
                    while (true) {
                        try self.validateFileName(param);
                        try self.files.append(self.alloc, param);
                        param = self.nextValue() orelse break;
                    }
                },
                .format => {
                    const param = try self.requireValue(token);

                    self.format = std.meta.stringToEnum(fmt.Format, param) orelse {
                        try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The format flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " is not a valid format (expected 'nul' or 'powershell')\n", .{param});
                        return error.InvalidFormat;
                    };
                },
                .required => {
                    var param = try self.requireValue(token);
                    while (true) {
                        try self.required.append(self.alloc, param);
                        param = self.nextValue() orelse break;
                    }
                },
                .scan => {
                    var param = try self.requireValue(token);
                    while (true) {
                        try self.scan.append(self.alloc, try utils.parseExt(param, self.logger));
                        param = self.nextValue() orelse break;
                    }
                },
                .version => {
                    try self.logger.print("nvi {s}\n", .{version});
                    return error.Version;
                },
            }
        }

        for (invalid_flags.items) |entry| {
            try self.logger.print(
                tty.yellow ++ "warning: " ++ tty.reset ++ "An unrecognized flag " ++ tty.bold_yellow ++ "{s}" ++ tty.reset,
                .{entry.flag},
            );

            if (entry.params.len > 0) {
                try self.logger.writeAll(" with parameters " ++ tty.bold_yellow);

                for (entry.params, 0..) |p, idx| {
                    if (idx != 0) try self.logger.writeByte(' ');
                    try self.logger.writeAll(p);
                }
            }

            try self.logger.writeAll(tty.reset ++ " was ignored \n");
        }

        if (self.files.items.len == 0) {
            try self.files.append(self.alloc, ".env");
        }

        if (self.debug) {
            try self.printFlags();
        }
    }

    fn next(self: *Arg) ?[:0]const u8 {
        if (self.i >= self.argv.len) return null;
        defer self.i += 1;

        return self.argv[self.i];
    }

    fn peek(self: *Arg) ?[:0]const u8 {
        return if (self.i < self.argv.len) self.argv[self.i] else null;
    }

    fn nextValue(self: *Arg) ?[:0]const u8 {
        const nxt = self.peek() orelse return null;

        return if (mem.startsWith(u8, nxt, "-")) null else self.next();
    }

    fn requireValue(self: *Arg, token: []const u8) ![:0]const u8 {
        return self.nextValue() orelse {
            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " Flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " requires at least 1 parameter\n", .{token});
            return error.MissingValue;
        };
    }

    fn rest(self: *Arg) []const [:0]const u8 {
        return self.argv[self.i..];
    }

    pub fn validateFileName(self: *Arg, f: []const u8) !void {
        const base = path.basename(f);
        const env_file = mem.eql(u8, base, ".env") or mem.startsWith(u8, base, ".env.") or mem.endsWith(u8, base, ".env");

        if (!env_file) {
            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The file flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " is not a valid env file (file must contain '.env' extension)\n", .{f});
            return error.InvalidFileExtension;
        }

        if (path.isAbsolute(f)) {
            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The file flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " must be relative to the current directory\n", .{f});
            return error.InvalidFilePath;
        }

        var it = mem.tokenizeScalar(u8, f, char.FORWARD_SLASH);
        while (it.next()) |component| {
            if (mem.eql(u8, component, "..")) {
                try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The file flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " may not escape the current directory\n", .{f});
                return error.InvalidFilePathEscape;
            }
        }
    }

    pub fn printMissingCommand(self: *Arg) void {
        self.logger.writeAll(tty.red ++ "error:" ++ tty.reset ++ " An " ++ tty.italic ++ "end of options delimiter" ++ tty.reset) catch {};
        self.logger.writeAll(" (--) must be defined and followed by a command (e.g., nvi <flags>" ++ tty.green ++ " -- <command>" ++ tty.reset ++ "). See nvi help.\n") catch {};
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

        try self.logger.writeAll("\n   ignored: ");
        if (self.ignored.items.len > 0) {
            for (self.ignored.items, 0..) |f, i| {
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
