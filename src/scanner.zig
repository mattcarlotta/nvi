const std = @import("std");
const tty = @import("tty.zig");
const arg = @import("arg.zig");
const utils = @import("utils.zig");
const ac = @import("accessors.zig");
const char = @import("char.zig");

const Io = std.Io;
const mem = std.mem;

pub const Scanner = struct {
    const blacklist = std.StaticStringMap(void).initComptime(.{
        .{ "node_modules", {} },
        .{ "bower_components", {} },
        .{ "storybook-static", {} },
        .{ "dist", {} },
        .{ "build", {} },
        .{ "out", {} },
        .{ "coverage", {} },
        .{ "target", {} },
        .{ "vendor", {} },
        .{ "zig-out", {} },
        .{ "__pycache__", {} },
        .{ "venv", {} },
        .{ "site-packages", {} },
        .{ "htmlcov", {} },
        .{ "_build", {} },
        .{ "deps", {} },
        .{ "dist-newstyle", {} },
        .{ "nimcache", {} },
        .{ "Pods", {} },
        .{ "Carthage", {} },
        .{ "DerivedData", {} },
        .{ "obj", {} },
    });

    pub const Match = struct {
        key: []const u8,
        line: usize,
        byte: usize,
    };

    const Extracted = struct {
        key: []const u8,
        key_off: usize,
        end: usize,
    };

    io: Io,
    alloc: mem.Allocator,
    args: *arg.Arg,
    logger: *Io.Writer,
    debug: bool = false,
    files_scanned: usize = 0,
    references: usize = 0,
    envs: std.StringArrayHashMapUnmanaged(usize) = .empty,

    pub fn scan(self: *Scanner) !void {
        if (self.debug) {
            try self.logger.writeAll(tty.cyan ++ "info: " ++ tty.reset ++ "Scanning for environment variables in");
            for (self.args.scan.items, 0..) |ext, i| {
                if (i != 0) try self.logger.writeAll(",");
                try self.logger.print(" " ++ tty.bold_green ++ "*.{s}" ++ tty.reset, .{ext});
            }
            try self.logger.writeAll(" files...\n\n");
        }

        var root = try Io.Dir.cwd().openDir(self.io, ".", .{ .iterate = true });
        defer root.close(self.io);

        try self.walk(root, "");

        if (self.debug) {
            try self.logger.print(
                tty.cyan ++ "info: " ++ tty.reset ++ "Scanned {d} file(s) and found {d} reference(s) to {d} unique key(s)\n\n",
                .{ self.files_scanned, self.references, self.envs.count() },
            );
        }

        try self.mergeRequired();
    }

    fn walk(self: *Scanner, dir: Io.Dir, prefix: []const u8) !void {
        var it = dir.iterate();
        while (try it.next(self.io)) |entry| {
            switch (entry.kind) {
                .directory => {
                    if ((entry.name.len > 0 and entry.name[0] == '.') or blacklist.has(entry.name)) continue;

                    var child = try dir.openDir(self.io, entry.name, .{ .iterate = true });
                    defer child.close(self.io);

                    const child_prefix = try std.fs.path.join(self.alloc, &.{ prefix, entry.name });
                    try self.walk(child, child_prefix);
                },
                .file => try self.scanFile(dir, prefix, entry.name),
                else => {},
            }
        }
    }

    fn scanFile(self: *Scanner, dir: Io.Dir, prefix: []const u8, name: []const u8) !void {
        const accessors = self.accessorsFor(name) orelse return;

        const contents = dir.readFileAlloc(self.io, name, self.alloc, .limited(10 * 1024 * 1024)) catch return;

        self.files_scanned += 1;

        var matches: std.ArrayList(Match) = .empty;
        defer matches.deinit(self.alloc);

        try self.scanContents(contents, accessors, &matches);
        if (matches.items.len == 0) return;

        if (self.args.debug) {
            try self.printMatches(prefix, name, matches.items);
        }

        for (matches.items) |m| {
            self.references += 1;
            const gop = try self.envs.getOrPut(self.alloc, try self.alloc.dupe(u8, m.key));
            if (!gop.found_existing) gop.value_ptr.* = 0;
            gop.value_ptr.* += 1;
        }
    }

    fn accessorsFor(self: *Scanner, name: []const u8) ?[]const ac.Accessor {
        const dot_ext = std.fs.path.extension(name);
        if (dot_ext.len < 2) return null;
        const ext = dot_ext[1..];

        var requested = false;
        for (self.args.scan.items) |e| {
            if (mem.eql(u8, e, ext)) {
                requested = true;
                break;
            }
        }
        if (!requested) return null;

        return ac.forExt(ext);
    }

    pub fn scanContents(
        self: *Scanner,
        contents: []const u8,
        accessors: []const ac.Accessor,
        matches: *std.ArrayList(Match),
    ) !void {
        var i: usize = 0;
        var line: usize = 1;
        var line_start: usize = 0;

        while (i < contents.len) {
            if (contents[i] == char.LINE_DELIMITER) {
                line += 1;
                line_start = i + 1;
                i += 1;
                continue;
            }

            const acc = self.matchPrefix(contents, i, accessors) orelse {
                i += 1;
                continue;
            };

            const key_start = i + acc.prefix.len;
            if (self.extractKey(contents, key_start, acc.extract)) |res| {
                try matches.append(self.alloc, .{
                    .key = res.key,
                    .line = line,
                    .byte = res.key_off - line_start + 1,
                });
                i = res.end;
            } else {
                i += acc.prefix.len;
            }
        }
    }

    fn matchPrefix(self: *Scanner, contents: []const u8, i: usize, accessors: []const ac.Accessor) ?ac.Accessor {
        _ = self;
        for (accessors) |acc| {
            if (!mem.startsWith(u8, contents[i..], acc.prefix)) continue;
            if (i > 0 and utils.isIdentChar(contents[i - 1]) and utils.isIdentChar(acc.prefix[0])) continue;
            return acc;
        }
        return null;
    }

    fn extractKey(self: *Scanner, contents: []const u8, start: usize, kind: ac.Extract) ?Extracted {
        _ = self;
        switch (kind) {
            // FOO immediately follows: process.env.FOO  $env:FOO
            .ident => {
                var e = start;
                while (e < contents.len and utils.isIdentChar(contents[e])) e += 1;
                if (e == start) return null;
                return .{ .key = contents[start..e], .key_off = start, .end = e };
            },
            // first string literal follows: getenv("FOO")  ENV['FOO']
            .quoted => {
                var p = start;
                while (p < contents.len and (contents[p] == ' ' or contents[p] == '\t')) p += 1;
                if (p >= contents.len) return null;

                const q = contents[p];
                if (q != '"' and q != '\'') return null; // dynamic key; skip

                const ks = p + 1;
                const close = mem.indexOfScalarPos(u8, contents, ks, q) orelse return null;
                if (sameLineOnly(contents, ks, close)) {} else return null;
                if (close == ks) return null; // empty string

                return .{ .key = contents[ks..close], .key_off = ks, .end = close + 1 };
            },
            // token up to '}', optional surrounding quotes: $ENV{FOO}  ${FOO}
            .braced => {
                const close = mem.indexOfScalarPos(u8, contents, start, '}') orelse return null;
                if (!sameLineOnly(contents, start, close)) return null;

                var ks = start;
                var ke = close;
                if (ke > ks and (contents[ks] == '\'' or contents[ks] == '"') and contents[ke - 1] == contents[ks]) {
                    ks += 1;
                    ke -= 1;
                }
                if (ke <= ks) return null;

                return .{ .key = contents[ks..ke], .key_off = ks, .end = close + 1 };
            },
            // bareword up to ')': $env(FOO)
            .parened => {
                const close = mem.indexOfScalarPos(u8, contents, start, ')') orelse return null;
                if (!sameLineOnly(contents, start, close)) return null;
                if (close == start) return null;

                return .{ .key = contents[start..close], .key_off = start, .end = close + 1 };
            },
        }
    }

    fn sameLineOnly(contents: []const u8, from: usize, to: usize) bool {
        return mem.indexOfScalarPos(u8, contents, from, '\n') == null or
            mem.indexOfScalarPos(u8, contents, from, '\n').? >= to;
    }

    fn isIgnored(self: *Scanner, key: []const u8) bool {
        for (self.args.ignored.items) |ig| {
            if (mem.eql(u8, ig, key)) return true;
        }
        return false;
    }

    pub fn mergeRequired(self: *Scanner) !void {
        if (self.args.command.len == 0) return;

        if (self.debug) try self.logger.writeAll(tty.cyan ++ "info: " ++ tty.reset ++ "The following ENV keys have been marked as required... \n");

        for (self.envs.keys()) |key| {
            if (self.isIgnored(key)) continue;
            try self.args.required.append(self.alloc, key);
            if (self.debug) try self.logger.print("    •" ++ tty.bold_green ++ " {s}" ++ tty.reset ++ "\n", .{key});
        }

        if (self.debug) try self.logger.writeByte('\n');
    }

    fn printMatches(self: *Scanner, prefix: []const u8, name: []const u8, matches: []const Match) !void {
        const rel = if (prefix.len == 0)
            name
        else
            try std.fs.path.join(self.alloc, &.{ prefix, name });

        try self.logger.print(
            tty.cyan ++ "info: " ++ tty.reset ++ "Scanned {s} and found {d} key(s)...\n",
            .{ rel, matches.len },
        );

        for (matches) |m| {
            try self.logger.print(
                "    • " ++ tty.bold_green ++ "{s}" ++ tty.reset ++ " (line {d}, byte {d})\n",
                .{ m.key, m.line, m.byte },
            );
        }

        try self.logger.writeByte('\n');
    }
};

pub fn scanFiles(io: Io, alloc: mem.Allocator, args: *arg.Arg, logger: *Io.Writer) !void {
    const debug = args.debug or args.command.len == 0;

    var scanner: Scanner = .{ .io = io, .alloc = alloc, .args = args, .debug = debug, .logger = logger };

    try scanner.scan();
}
