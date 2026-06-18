const std = @import("std");
const Io = std.Io;
const mem = std.mem;
const path = std.fs.path;
const builtin = @import("builtin");
const version = @import("build_options").version;

const char = @import("char.zig");
const fmt = @import("formatter.zig");
const tty = @import("tty.zig");
const utils = @import("utils.zig");

// Parses argv into the Arg config that drives the rest of the pipeline: files,
// required/ignored keys, scan extensions, output format, dry-run, and the
// command after `--`.
//
// Flags accept multiple values until the next dash-prefixed
// token.
//
// `help`/`version` short-circuit via error.Help/error.Version. Emits its
// own usage diagnostics, so main only maps errors to exit codes.

const DEFAULT_ENV_FILE: *const [4:0]u8 = ".env";

const IgnoredFlag = struct {
    flag: []const u8,
    params: []const [:0]const u8 = &.{},
};

pub const Arg = struct {
    alloc: mem.Allocator,
    argv: []const [:0]const u8,
    // skipping program name at argv[0]
    i: usize = 1,
    files: std.ArrayList([]const u8) = .empty,
    command: []const [:0]const u8 = &.{},
    dry_run: bool = false,
    format: fmt.Format = .default,
    required: std.ArrayList([]const u8) = .empty,
    ignored: std.ArrayList([]const u8) = .empty,
    scan: std.ArrayList([]const u8) = .empty,
    logger: *Io.Writer,

    const Flag = enum { files, dry_run, format, ignored, required, scan, help, version };

    const flags = std.StaticStringMap(Flag).initComptime(.{
        .{ "-f", .files },
        .{ "--files", .files },

        .{ "-d", .dry_run },
        .{ "--dry-run", .dry_run },

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

        while (self.next()) |token| {
            if (mem.eql(u8, token, "--")) {
                const cmd = self.rest();

                if (self.dry_run) {
                    try self.logger.writeAll(tty.yellow ++ "warning" ++ tty.reset ++ ": The " ++ tty.bold_yellow);
                    for (cmd, 0..) |part, i| {
                        if (i != 0) try self.logger.writeByte(' ');
                        try self.logger.writeAll(part);
                    }
                    try self.logger.writeAll(tty.reset ++ " command will not be emitted in dry-run mode; skipped.\n\n");
                    break;
                }

                self.command = cmd;
                break;
            }

            const flag = Arg.flags.get(token) orelse {
                const start = self.i;

                self.skipToNextFlag();

                try invalid_flags.append(self.alloc, .{ .flag = token, .params = self.argv[start..self.i] });

                continue;
            };

            switch (flag) {
                .dry_run => self.dry_run = true,
                .help => {
                    try utils.printHelp(self.logger);
                    return error.Help;
                },
                .ignored => {
                    var param: ?[:0]const u8 = try self.requireValue(token);

                    while (param) |p| : (param = self.nextValue()) {
                        try self.ignored.append(self.alloc, p);
                    }
                },
                .files => {
                    var param: ?[:0]const u8 = try self.requireValue(token);

                    while (param) |p| : (param = self.nextValue()) {
                        try self.validateFileName(p);
                        try self.files.append(self.alloc, p);
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
                    var param: ?[:0]const u8 = try self.requireValue(token);

                    while (param) |p| : (param = self.nextValue()) {
                        try self.required.append(self.alloc, p);
                    }
                },
                .scan => {
                    var param: ?[:0]const u8 = try self.requireValue(token);

                    while (param) |p| : (param = self.nextValue()) {
                        const ext = utils.parseExt(p) catch |err| {
                            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " The scan parameter " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " is not a file extension (e.g. mjs or *.mjs). If you passed an unquoted glob, your shell may have expanded it into filenames; pass the bare extension instead\n", .{p});
                            return err;
                        };
                        try self.scan.append(self.alloc, ext);
                    }
                },
                .version => {
                    try self.logger.print("nvi {s}\n", .{version});
                    try self.logger.print("Build type: {s}\n", .{@tagName(builtin.mode)});
                    try self.logger.print("Zig {f}\n", .{builtin.zig_version});
                    try self.logger.print("Target: {s}-{s}\n", .{
                        @tagName(builtin.target.cpu.arch),
                        @tagName(builtin.target.os.tag),
                    });
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
            try self.files.append(self.alloc, DEFAULT_ENV_FILE);
        }

        if (self.dry_run) {
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

        return if (nxt.len > 0 and nxt[0] == char.DASH) null else self.next();
    }

    fn skipToNextFlag(self: *Arg) void {
        while (self.nextValue()) |_| {}
    }

    fn requireValue(self: *Arg, token: []const u8) ![:0]const u8 {
        return self.nextValue() orelse {
            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " Flag " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " requires at least 1 parameter\n", .{token});
            return error.MissingValue;
        };
    }

    fn rest(self: *Arg) []const [:0]const u8 {
        defer self.i = self.argv.len - 1;

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
            try self.logger.writeAll(tty.dark_gray ++ "(undefined)" ++ tty.reset);
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
            try self.logger.writeAll(tty.dark_gray ++ "(undefined)" ++ tty.reset);
        }

        try self.logger.writeAll("\n   ignored: ");
        if (self.ignored.items.len > 0) {
            for (self.ignored.items, 0..) |f, i| {
                if (i != 0) try self.logger.writeAll(", ");
                try self.logger.print(tty.bold_green ++ "{s}" ++ tty.reset, .{f});
            }
        } else {
            try self.logger.writeAll(tty.dark_gray ++ "(undefined)" ++ tty.reset);
        }

        try self.logger.writeAll("\n   scan: ");
        if (self.scan.items.len > 0) {
            for (self.scan.items, 0..) |f, i| {
                if (i != 0) try self.logger.writeAll(", ");
                try self.logger.print(tty.bold_green ++ "{s}" ++ tty.reset, .{f});
            }
        } else {
            try self.logger.writeAll(tty.dark_gray ++ "(undefined)" ++ tty.reset);
        }

        try self.logger.print("\n   format: " ++ tty.bold_green ++ "{t}" ++ tty.reset, .{self.format});

        try self.logger.writeAll("\n\n");
    }
};
