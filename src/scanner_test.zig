const std = @import("std");
const arg = @import("arg.zig");
const sc = @import("scanner.zig");

const Io = std.Io;
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;

const TestScan = struct {
    arena: std.heap.ArenaAllocator,
    matches: std.ArrayList(sc.Match) = .empty,

    fn init() TestScan {
        return .{ .arena = std.heap.ArenaAllocator.init(std.testing.allocator) };
    }

    fn deinit(self: *TestScan) void {
        self.arena.deinit();
    }

    fn run(self: *TestScan, contents: []const u8) ![]const sc.Match {
        try sc.scanContents(self.arena.allocator(), contents, &self.matches);
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
    try expect(sc.matchesExt("main.zig", &.{"zig"}));
    try expect(sc.matchesExt("a.test.ts", &.{ "zig", "ts" }));
    try expect(!sc.matchesExt("main.zig", &.{"ts"}));
    try expect(!sc.matchesExt("Makefile", &.{"zig"}));
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
    try scanner.counts.put(alloc, "NODE_ENV", 3);
    try scanner.counts.put(alloc, "API_KEY_ENV", 1);

    try scanner.mergeRequired();

    try expectEqual(@as(usize, 1), args.required.items.len);
    try expectEqualStrings("API_KEY_ENV", args.required.items[0]);
}
