const std = @import("std");
const tty = @import("tty.zig");
const arg = @import("arg.zig");

const Io = std.Io;
const mem = std.mem;
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;

const SUFFIX = "_ENV";

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

pub const Match = struct {
    key: []const u8,
    line: usize,
    byte: usize,
};

fn isIdentChar(c: u8) bool {
    return std.ascii.isAlphanumeric(c) or c == '_';
}

fn isKeyChar(c: u8) bool {
    return (c >= 'A' and c <= 'Z') or (c >= '0' and c <= '9') or c == '_';
}

fn scanContents(
    alloc: mem.Allocator,
    contents: []const u8,
    matches: *std.ArrayList(Match),
) !void {
    var pos: usize = 0;
    while (mem.indexOfPos(u8, contents, pos, SUFFIX)) |i| {
        const end = i + SUFFIX.len;

        // suffix continues into a larger identifier (THING_ENVIRONMENT, A_ENV_B)
        if (end < contents.len and isIdentChar(contents[end])) {
            pos = i + 1;
            continue;
        }

        // walk back to the start of the key
        var start = i;
        while (start > 0 and isKeyChar(contents[start - 1])) start -= 1;

        // a bare "_ENV" with no key chars before it isn't a key
        if (start == i) {
            pos = end;
            continue;
        }

        // key is the tail of a larger mixed-case identifier (my_API_ENV)
        if (start > 0 and isIdentChar(contents[start - 1])) {
            pos = end;
            continue;
        }

        const line_start = if (mem.lastIndexOfScalar(u8, contents[0..start], '\n')) |nl| nl + 1 else 0;

        try matches.append(alloc, .{
            .key = contents[start..end],
            .line = mem.count(u8, contents[0..start], "\n") + 1,
            .byte = start - line_start + 1,
        });

        pos = end;
    }
}

fn matchesExt(name: []const u8, exts: []const []const u8) bool {
    const dot_ext = std.fs.path.extension(name);
    if (dot_ext.len < 2) return false;

    for (exts) |ext| {
        if (mem.eql(u8, dot_ext[1..], ext)) return true;
    }
    return false;
}

const Totals = struct {
    references: usize = 0,
    counts: std.StringArrayHashMapUnmanaged(usize) = .empty,
};

fn walkTree(
    io: Io,
    alloc: mem.Allocator,
    dir: Io.Dir,
    prefix: []const u8,
    args: *const arg.Arg,
    totals: *Totals,
    logger: *Io.Writer,
) !void {
    var it = dir.iterate();
    while (try it.next(io)) |entry| {
        switch (entry.kind) {
            .directory => {
                if (blacklist.has(entry.name)) continue;

                var child = try dir.openDir(io, entry.name, .{ .iterate = true });
                defer child.close(io);

                const child_prefix = try std.fs.path.join(alloc, &.{ prefix, entry.name });
                try walkTree(io, alloc, child, child_prefix, args, totals, logger);
            },
            .file => {
                if (!matchesExt(entry.name, args.scan.items)) continue;

                const contents = dir.readFileAlloc(io, entry.name, alloc, .limited(10 * 1024 * 1024)) catch continue;

                var matches: std.ArrayList(Match) = .empty;
                defer matches.deinit(alloc);

                try scanContents(alloc, contents, &matches);
                if (matches.items.len == 0) continue;

                if (args.debug) {
                    const rel = if (prefix.len == 0)
                        entry.name
                    else
                        try std.fs.path.join(alloc, &.{ prefix, entry.name });

                    try logger.print(
                        tty.cyan ++ "info: " ++ tty.reset ++ "Scanned {s} and found {d} key(s)...\n",
                        .{ rel, matches.items.len },
                    );

                    for (matches.items) |m| {
                        try logger.print(
                            "    • " ++ tty.bold_green ++ "{s}" ++ tty.reset ++ " (line {d}, byte {d})\n",
                            .{ m.key, m.line, m.byte },
                        );
                    }

                    try logger.writeByte('\n');
                }

                for (matches.items) |m| {
                    totals.references += 1;
                    const gop = try totals.counts.getOrPut(alloc, try alloc.dupe(u8, m.key));
                    if (!gop.found_existing) gop.value_ptr.* = 0;
                    gop.value_ptr.* += 1;
                }
            },
            else => {},
        }
    }
}

fn isIgnored(args: *const arg.Arg, key: []const u8) bool {
    for (args.ignored.items) |ig| {
        if (mem.eql(u8, ig, key)) return true;
    }
    return false;
}

fn mergeRequired(alloc: mem.Allocator, args: *arg.Arg, counts: *const std.StringArrayHashMapUnmanaged(usize)) !void {
    for (counts.keys()) |key| {
        if (isIgnored(args, key)) continue;
        try args.required.append(alloc, key);
    }
}

pub fn scanFiles(io: Io, alloc: mem.Allocator, args: *arg.Arg, logger: *Io.Writer) !void {
    if (args.debug) {
        try logger.writeAll(tty.cyan ++ "info: " ++ tty.reset ++ "Scanning for *" ++ SUFFIX ++ " keys in");
        for (args.scan.items, 0..) |ext, i| {
            if (i != 0) try logger.writeAll(",");
            try logger.print(" " ++ tty.bold_green ++ "*.{s}" ++ tty.reset, .{ext});
        }
        try logger.writeAll(" files...\n\n");
    }

    var totals: Totals = .{};

    var root = try Io.Dir.cwd().openDir(io, ".", .{ .iterate = true });
    defer root.close(io);

    try walkTree(io, alloc, root, "", args, &totals, logger);

    if (args.debug) {
        try logger.print(
            tty.cyan ++ "info: " ++ tty.reset ++ "Found {d} reference(s) to {d} unique key(s)\n\n",
            .{ totals.references, totals.counts.count() },
        );
    }

    try mergeRequired(alloc, args, &totals.counts);
}

const TestScan = struct {
    arena: std.heap.ArenaAllocator,
    matches: std.ArrayList(Match) = .empty,

    fn init() TestScan {
        return .{ .arena = std.heap.ArenaAllocator.init(std.testing.allocator) };
    }

    fn deinit(self: *TestScan) void {
        self.arena.deinit();
    }

    fn run(self: *TestScan, contents: []const u8) ![]const Match {
        try scanContents(self.arena.allocator(), contents, &self.matches);
        return self.matches.items;
    }
};

test "scanContents finds a key with location" {
    var t = TestScan.init();
    defer t.deinit();

    const matches = try t.run("const k = env.get(\"API_KEY_ENV\");\n");
    try expectEqual(@as(usize, 1), matches.len);
    try expectEqualStrings("API_KEY_ENV", matches[0].key);
    try expectEqual(@as(usize, 1), matches[0].line);
    try expectEqual(@as(usize, 20), matches[0].byte);
}

test "scanContents tracks lines and finds multiple keys" {
    var t = TestScan.init();
    defer t.deinit();

    const matches = try t.run("x\ny\nA_ENV and B_ENV\n");
    try expectEqual(@as(usize, 2), matches.len);
    try expectEqualStrings("A_ENV", matches[0].key);
    try expectEqual(@as(usize, 3), matches[0].line);
    try expectEqual(@as(usize, 1), matches[0].byte);
    try expectEqualStrings("B_ENV", matches[1].key);
    try expectEqual(@as(usize, 11), matches[1].byte);
}

test "scanContents skips mid-identifier, bare, and mixed-case-tail matches" {
    var t = TestScan.init();
    defer t.deinit();

    const matches = try t.run("THING_ENVIRONMENT A_ENV_B my_ENV \"_ENV\" OK_ENV\n");
    try expectEqual(@as(usize, 1), matches.len);
    try expectEqualStrings("OK_ENV", matches[0].key);
}

test "matchesExt compares the file extension" {
    try expect(matchesExt("main.zig", &.{"zig"}));
    try expect(matchesExt("a.test.ts", &.{ "zig", "ts" }));
    try expect(!matchesExt("main.zig", &.{"ts"}));
    try expect(!matchesExt("Makefile", &.{"zig"}));
}

test "mergeRequired skips ignored keys" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    var logger_buf: [256]u8 = undefined;
    var logger: Io.Writer = .fixed(&logger_buf);
    var args: arg.Arg = .{ .logger = &logger };
    try args.ignored.append(alloc, "NODE_ENV");

    var counts: std.StringArrayHashMapUnmanaged(usize) = .empty;
    try counts.put(alloc, "NODE_ENV", 3);
    try counts.put(alloc, "API_KEY_ENV", 1);

    try mergeRequired(alloc, &args, &counts);

    try expectEqual(@as(usize, 1), args.required.items.len);
    try expectEqualStrings("API_KEY_ENV", args.required.items[0]);
}
