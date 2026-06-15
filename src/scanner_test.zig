const std = @import("std");
const arg = @import("arg.zig");
const sc = @import("scanner.zig");
const ac = @import("accessors.zig");

const Io = std.Io;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;

const js = [_]ac.Accessor{
    .{ .prefix = "process.env.", .extract = .ident },
    .{ .prefix = "process.env[", .extract = .quoted },
};
const py = [_]ac.Accessor{
    .{ .prefix = "os.getenv(", .extract = .quoted },
};
const perl = [_]ac.Accessor{
    .{ .prefix = "$ENV{", .extract = .braced },
};

const TestScan = struct {
    arena: std.heap.ArenaAllocator,
    matches: std.ArrayList(sc.Scanner.Match) = .empty,

    fn init() TestScan {
        return .{ .arena = std.heap.ArenaAllocator.init(std.testing.allocator) };
    }

    fn deinit(self: *TestScan) void {
        self.arena.deinit();
    }

    fn run(self: *TestScan, contents: []const u8, accessors: []const ac.Accessor) ![]const sc.Scanner.Match {
        var scanner: sc.Scanner = .{
            .io = undefined,
            .alloc = self.arena.allocator(),
            .args = undefined,
            .logger = undefined,
        };
        try scanner.scanContents(contents, accessors, &self.matches);
        return self.matches.items;
    }
};

test "scanContents extracts an ident key with location" {
    var t = TestScan.init();
    defer t.deinit();

    const matches = try t.run("const k = process.env.API_KEY;\n", &js);
    try expectEqual(@as(usize, 1), matches.len);
    try expectEqualStrings("API_KEY", matches[0].key);
    try expectEqual(@as(usize, 1), matches[0].line);
    try expectEqual(@as(usize, 23), matches[0].byte);
}

test "scanContents extracts a bracket-quoted key with location" {
    var t = TestScan.init();
    defer t.deinit();

    const matches = try t.run("const k = process.env[\"API_KEY\"];\n", &js);
    try expectEqual(@as(usize, 1), matches.len);
    try expectEqualStrings("API_KEY", matches[0].key);
    try expectEqual(@as(usize, 1), matches[0].line);
    try expectEqual(@as(usize, 24), matches[0].byte);
}

test "scanContents tracks lines across multiple quoted keys" {
    var t = TestScan.init();
    defer t.deinit();

    const matches = try t.run("\n\nos.getenv(\"FOO\")\nos.getenv(\"BAR\")\n", &py);
    try expectEqual(@as(usize, 2), matches.len);
    try expectEqualStrings("FOO", matches[0].key);
    try expectEqual(@as(usize, 3), matches[0].line);
    try expectEqual(@as(usize, 12), matches[0].byte);
    try expectEqualStrings("BAR", matches[1].key);
    try expectEqual(@as(usize, 4), matches[1].line);
    try expectEqual(@as(usize, 12), matches[1].byte);
}

test "scanContents skips dynamic (non-literal) keys" {
    var t = TestScan.init();
    defer t.deinit();

    const matches = try t.run("os.getenv(name) os.getenv(\"REAL\")\n", &py);
    try expectEqual(@as(usize, 1), matches.len);
    try expectEqualStrings("REAL", matches[0].key);
}

test "scanContents skips a prefix that begins mid-identifier" {
    var t = TestScan.init();
    defer t.deinit();

    const matches = try t.run("xprocess.env.FAKE process.env.REAL\n", &js);
    try expectEqual(@as(usize, 1), matches.len);
    try expectEqualStrings("REAL", matches[0].key);
}

test "scanContents strips optional quotes inside a braced key" {
    var t = TestScan.init();
    defer t.deinit();

    const matches = try t.run("$ENV{FOO} $ENV{'BAR'}\n", &perl);
    try expectEqual(@as(usize, 2), matches.len);
    try expectEqualStrings("FOO", matches[0].key);
    try expectEqualStrings("BAR", matches[1].key);
}

test "scanContents works with a real table from accessors.forExt" {
    var t = TestScan.init();
    defer t.deinit();

    const accessors = ac.forExt("ts").?;
    const matches = try t.run("const u = process.env.DATABASE_URL;\n", accessors);
    try expectEqual(@as(usize, 1), matches.len);
    try expectEqualStrings("DATABASE_URL", matches[0].key);
}

test "mergeRequired skips ignored keys" {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    var logger_buf: [256]u8 = undefined;
    var logger: Io.Writer = .fixed(&logger_buf);
    var args: arg.Arg = .{ .argv = &.{}, .command = &.{"test"}, .logger = &logger };
    try args.ignored.append(alloc, "NODE_ENV");

    var scanner: sc.Scanner = .{ .io = undefined, .alloc = alloc, .args = &args, .logger = &logger };
    try scanner.envs.put(alloc, "NODE_ENV", 3);
    try scanner.envs.put(alloc, "API_KEY", 1);

    try scanner.mergeRequired();

    try expectEqual(@as(usize, 1), args.required.items.len);
    try expectEqualStrings("API_KEY", args.required.items[0]);
}
