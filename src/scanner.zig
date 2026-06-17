const std = @import("std");
const Io = std.Io;
const mem = std.mem;
const path = std.fs.path;

const ac = @import("accessors.zig");
const arg = @import("arg.zig");
const char = @import("char.zig");
const tty = @import("tty.zig");
const utils = @import("utils.zig");

// Recursively walks the working directory and collects environment-variable
// keys referenced in source files, so they can be marked required.
//
// For each file whose extension maps to a set of accessors (see accessors.zig), the
// contents are read once into memory, scanned linearly for accessor prefixes,
// and each match's key is extracted and counted. The unique keys are then
// appended to the required list (minus any the user ignored).
//
// Files are read and closed one at a time; only the current root-to-leaf chain
// of directory handles is open at once. Blacklisted and dot directories are
// skipped. Files over 10MB are skipped with a warning, since a silently
// unscanned file could hide a key.
//
// Without a command, scan runs as a report (dry-run) and returns error.NoCommand.

const Env = struct {
    key: []const u8,
    start: usize,
    end: usize,
};

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

    io: Io,
    alloc: mem.Allocator,
    args: *arg.Arg,
    logger: *Io.Writer,
    dry_run: bool = false,
    files_scanned: usize = 0,
    references: usize = 0,
    envs: std.StringArrayHashMapUnmanaged(usize) = .empty,
    exts: std.StringArrayHashMapUnmanaged([]const ac.Accessor) = .empty,

    pub fn run(self: *Scanner) !void {
        self.dry_run = self.args.dry_run or self.args.command.len == 0;

        if (self.dry_run) {
            try self.logger.writeAll(tty.cyan ++ "info: " ++ tty.reset ++ "Scanning for environment variables in");

            for (self.args.scan.items, 0..) |ext, i| {
                if (i != 0) try self.logger.writeAll(",");
                try self.logger.print(" " ++ tty.bold_green ++ "*.{s}" ++ tty.reset, .{ext});
            }

            try self.logger.writeAll(" files...\n\n");
        }

        for (self.args.scan.items) |ext| {
            if (ac.extensions.get(ext)) |accessors| {
                try self.exts.put(self.alloc, ext, accessors);
                continue;
            }

            if (self.dry_run) {
                try self.logger.print(
                    tty.yellow ++ "warning: " ++ tty.reset ++ "No accessor patterns were found for " ++ tty.bold_yellow ++ "*.{s}" ++ tty.reset ++ ", skipping.\n\n",
                    .{ext},
                );
            }
        }

        var root = try Io.Dir.cwd().openDir(self.io, ".", .{ .iterate = true });
        defer root.close(self.io);

        try self.walkFileTree(root, "");

        if (self.dry_run) {
            try self.logger.print(
                tty.cyan ++ "info: " ++ tty.reset ++ "Scanned {d} file(s) and found {d} reference(s) to {d} unique key(s)\n\n",
                .{ self.files_scanned, self.references, self.envs.count() },
            );
        }

        try self.mergeRequiredEnvs();

        if (self.args.command.len == 0) return error.NoCommand;
    }

    fn walkFileTree(self: *Scanner, dir: Io.Dir, prefix: []const u8) !void {
        var it = dir.iterate();
        while (try it.next(self.io)) |entry| {
            switch (entry.kind) {
                .directory => {
                    if (mem.startsWith(u8, entry.name, ".") or blacklist.has(entry.name)) continue;

                    var child = try dir.openDir(self.io, entry.name, .{ .iterate = true });
                    defer child.close(self.io);

                    const child_prefix = try path.join(self.alloc, &.{ prefix, entry.name });
                    try self.walkFileTree(child, child_prefix);
                },
                .file => try self.scanFile(dir, prefix, entry.name),
                else => {},
            }
        }
    }

    fn scanFile(self: *Scanner, dir: Io.Dir, prefix: []const u8, name: []const u8) !void {
        const accessors = self.getAccessors(name) orelse return;

        const contents = dir.readFileAlloc(self.io, name, self.alloc, .limited(10 * 1024 * 1024)) catch {
            try self.logger.print(
                tty.yellow ++ "warning: " ++ tty.reset ++ "The " ++ tty.bold_yellow ++ "{s}" ++ tty.reset ++ " file exceeds 10MB, skipping.\n",
                .{name},
            );
            return;
        };

        self.files_scanned += 1;

        var matches: std.ArrayList(Match) = .empty;
        defer matches.deinit(self.alloc);

        try self.scanContents(contents, accessors, &matches);

        if (matches.items.len == 0) return;

        if (self.dry_run) {
            try self.printMatches(prefix, name, matches.items);
        }

        for (matches.items) |m| {
            self.references += 1;

            const env = try self.envs.getOrPut(self.alloc, m.key);
            if (!env.found_existing) {
                env.key_ptr.* = try self.alloc.dupe(u8, m.key);
                env.value_ptr.* = 0;
            }

            env.value_ptr.* += 1;
        }
    }

    fn getAccessors(self: *Scanner, name: []const u8) ?[]const ac.Accessor {
        const dot_ext = path.extension(name);

        if (dot_ext.len < 2) return null;

        return self.exts.get(dot_ext[1..]);
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

            if (extractKey(contents, i + acc.prefix.len, acc.pattern)) |env| {
                try matches.append(self.alloc, .{
                    .key = env.key,
                    .line = line,
                    .byte = env.start - line_start + 1,
                });

                i = env.end;
                continue;
            }

            i += acc.prefix.len;
        }
    }

    fn matchPrefix(_: *Scanner, contents: []const u8, i: usize, accessors: []const ac.Accessor) ?ac.Accessor {
        for (accessors) |acc| {
            if (!mem.startsWith(u8, contents[i..], acc.prefix)) continue;

            if (i > 0 and utils.isIdentChar(contents[i - 1]) and utils.isIdentChar(acc.prefix[0])) continue;

            return acc;
        }

        return null;
    }

    fn isIgnoredKey(self: *Scanner, key: []const u8) bool {
        for (self.args.ignored.items) |ig| {
            if (mem.eql(u8, ig, key)) return true;
        }

        return false;
    }

    pub fn mergeRequiredEnvs(self: *Scanner) !void {
        if (self.envs.count() == 0) return;

        if (self.dry_run) try self.logger.writeAll(tty.cyan ++ "info: " ++ tty.reset ++ "The following ENV keys will be marked as required... \n");

        for (self.envs.keys()) |key| {
            if (self.isIgnoredKey(key)) continue;

            try self.args.required.append(self.alloc, key);

            if (self.dry_run) try self.logger.print("    •" ++ tty.bold_green ++ " {s}" ++ tty.reset ++ "\n", .{key});
        }

        if (self.dry_run) try self.logger.writeByte('\n');
    }

    fn printMatches(self: *Scanner, prefix: []const u8, name: []const u8, matches: []const Match) !void {
        const file = if (prefix.len == 0) name else try path.join(self.alloc, &.{ prefix, name });

        try self.logger.print(
            tty.cyan ++ "info: " ++ tty.reset ++ "Scanned " ++ tty.green ++ "{s}" ++ tty.reset ++ " and found {d} key(s)...\n",
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
};

fn extractKey(contents: []const u8, start: usize, kind: ac.Pattern) ?Env {
    switch (kind) {
        .ident => {
            var end = start;

            while (end < contents.len and utils.isIdentChar(contents[end])) end += 1;

            if (end == start) return null;

            return .{ .key = contents[start..end], .start = start, .end = end };
        },
        .quoted => {
            var cursor = start;

            while (cursor < contents.len and (contents[cursor] == char.SPACE or contents[cursor] == char.TAB)) cursor += 1;

            if (cursor >= contents.len) return null;

            const quote = contents[cursor];
            if (quote != char.DOUBLE_QUOTE and quote != char.SINGLE_QUOTE) return null;

            const key_start = cursor + 1;
            const key_end = mem.indexOfScalarPos(u8, contents, key_start, quote) orelse return null;

            if (!utils.sameLineOnly(contents, key_start, key_end)) return null;

            // empty quotes
            if (key_end == key_start) return null;

            return .{ .key = contents[key_start..key_end], .start = key_start, .end = key_end + 1 };
        },
        .braced => {
            const brace = mem.indexOfScalarPos(u8, contents, start, char.CLOSE_BRACE) orelse return null;
            if (!utils.sameLineOnly(contents, start, brace)) return null;

            var key_start = start;
            var key_end = brace;
            // strip a matching pair of surrounding quotes: $ENV{'FOO'} -> FOO
            if (key_end > key_start and
                (contents[key_start] == char.SINGLE_QUOTE or contents[key_start] == char.DOUBLE_QUOTE) and
                contents[key_end - 1] == contents[key_start])
            {
                key_start += 1;
                key_end -= 1;
            }

            if (key_end <= key_start) return null;

            return .{ .key = contents[key_start..key_end], .start = key_start, .end = brace + 1 };
        },
        .parened => {
            const paren = mem.indexOfScalarPos(u8, contents, start, char.CLOSE_PAREN) orelse return null;

            if (!utils.sameLineOnly(contents, start, paren)) return null;

            if (paren == start) return null;

            return .{ .key = contents[start..paren], .start = start, .end = paren + 1 };
        },
    }
}
