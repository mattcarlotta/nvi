const std = @import("std");
const tty = @import("tty.zig");
const arg = @import("arg.zig");
const utils = @import("utils.zig");

const Io = std.Io;
const mem = std.mem;

const SUFFIX = "_ENV";

pub const Match = struct {
    key: []const u8,
    line: usize,
    byte: usize,
};

pub const Scanner = struct {
    const blacklist = std.StaticStringMap(void).initComptime(.{
        .{ ".git", {} },
        .{ "node_modules", {} },
        .{ "zig-out", {} },
        .{ ".zig-cache", {} },
        .{ "zig-cache", {} },
        .{ "dist", {} },
        .{ "build", {} },
        .{ "vendor", {} },
        .{ "target", {} },
        .{ "coverage", {} },
    });

    io: Io,
    alloc: mem.Allocator,
    args: *arg.Arg,
    logger: *Io.Writer,
    debug: bool = false,
    files_scanned: usize = 0,
    references: usize = 0,
    envs: std.StringArrayHashMapUnmanaged(usize) = .empty,

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
        const file = if (prefix.len == 0)
            name
        else
            try std.fs.path.join(self.alloc, &.{ prefix, name });

        try self.logger.print(
            tty.cyan ++ "info: " ++ tty.reset ++ "Scanned {s} and found {d} key(s)...\n",
            .{ file, matches.len },
        );

        for (matches) |m| {
            try self.logger.print(
                "    • " ++ tty.bold_green ++ "{s}" ++ tty.reset ++ " (line {d}, byte {d})\n",
                .{ m.key, m.line, m.byte },
            );
        }

        try self.logger.writeByte('\n');
    }

    pub fn scanContents(self: *Scanner, contents: []const u8, matches: *std.ArrayList(Match)) !void {
        var pos: usize = 0;
        while (mem.indexOfPos(u8, contents, pos, SUFFIX)) |i| {
            const end = i + SUFFIX.len;

            // suffix continues into a larger identifier (THING_ENVIRONMENT, A_ENV_B)
            if (end < contents.len and utils.isIdentChar(contents[end])) {
                pos = i + 1;
                continue;
            }

            var start = i;
            while (start > 0 and utils.isKeyChar(contents[start - 1])) start -= 1;

            // a bare "_ENV" with no key chars before it isn't a key
            if (start == i) {
                pos = end;
                continue;
            }

            // key is the tail of a larger mixed-case identifier (my_API_ENV)
            if (start > 0 and utils.isIdentChar(contents[start - 1])) {
                pos = end;
                continue;
            }

            const line_start = if (mem.lastIndexOfScalar(u8, contents[0..start], '\n')) |nl| nl + 1 else 0;

            try matches.append(self.alloc, .{
                .key = contents[start..end],
                .line = mem.count(u8, contents[0..start], "\n") + 1,
                .byte = start - line_start + 1,
            });

            pos = end;
        }
    }

    pub fn matchesExt(self: *Scanner, name: []const u8) bool {
        const dot_ext = std.fs.path.extension(name);
        if (dot_ext.len < 2) return false;

        for (self.args.scan.items) |ext| {
            if (mem.eql(u8, dot_ext[1..], ext)) return true;
        }

        return false;
    }

    fn scanFile(self: *Scanner, dir: Io.Dir, prefix: []const u8, name: []const u8) !void {
        if (!self.matchesExt(name)) return;

        const contents = dir.readFileAlloc(self.io, name, self.alloc, .limited(10 * 1024 * 1024)) catch {
            try self.logger.print(tty.yellow ++ "warning: " ++ tty.reset ++ "The file " ++ tty.bold_yellow ++ "{s}" ++ tty.reset ++ " exceeds 10Mb; skipping.", .{name});
            return;
        };

        self.files_scanned += 1;

        var matches: std.ArrayList(Match) = .empty;
        defer matches.deinit(self.alloc);

        try self.scanContents(contents, &matches);

        if (matches.items.len == 0) return;

        if (self.args.debug) {
            try self.printMatches(prefix, name, matches.items);
        }

        for (matches.items) |m| {
            self.references += 1;
            const env = try self.envs.getOrPut(self.alloc, try self.alloc.dupe(u8, m.key));
            if (!env.found_existing) env.value_ptr.* = 0;
            env.value_ptr.* += 1;
        }
    }

    fn walk(self: *Scanner, dir: Io.Dir, prefix: []const u8) !void {
        var it = dir.iterate();
        while (try it.next(self.io)) |entry| {
            switch (entry.kind) {
                .directory => {
                    if (blacklist.has(entry.name)) continue;

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

    pub fn scan(self: *Scanner) !void {
        if (self.debug) {
            try self.logger.writeAll(tty.cyan ++ "info: " ++ tty.reset ++ "Scanning for *" ++ SUFFIX ++ " keys in");
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
};

pub fn scanFiles(io: Io, alloc: mem.Allocator, args: *arg.Arg, logger: *Io.Writer) !void {
    var scanner: Scanner = .{ .io = io, .alloc = alloc, .args = args, .debug = args.debug or args.command.len == 0, .logger = logger };

    try scanner.scan();
}
