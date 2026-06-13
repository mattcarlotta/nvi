const std = @import("std");
const arg = @import("arg.zig");
const token = @import("tokenizer.zig");
const p = @import("parser.zig");

const Io = std.Io;
const Environ = std.process.Environ;
const indexOf = std.mem.indexOf;
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;
const expectError = std.testing.expectError;

const TestParse = struct {
    arena: std.heap.ArenaAllocator,
    logger_buf: [4096]u8 = undefined,
    logger: Io.Writer = undefined,
    environ: Environ.Map = undefined,
    args: arg.Arg = undefined,
    tokens: std.ArrayList(token.Token) = .empty,

    fn init(self: *TestParse) void {
        self.* = .{ .arena = std.heap.ArenaAllocator.init(std.testing.allocator) };
        const a = self.arena.allocator();
        self.logger = .fixed(&self.logger_buf);
        self.environ = Environ.Map.init(a);
        self.args = .{ .argv = &.{}, .logger = &self.logger };
    }

    fn deinit(self: *TestParse) void {
        self.arena.deinit();
    }

    fn alloc(self: *TestParse) std.mem.Allocator {
        return self.arena.allocator();
    }

    fn addToken(self: *TestParse, key: ?[]const u8, kind: token.ValueKind, value: []const u8) !void {
        var tok: token.Token = .{ .key = key, .file = "testp.env" };
        try tok.values.append(self.alloc(), .{ .kind = kind, .value = value, .line = 1, .byte = 1 });
        try self.tokens.append(self.alloc(), tok);
    }

    fn output(self: *TestParse) []const u8 {
        return self.logger.buffered();
    }

    fn run(self: *TestParse) !std.StringArrayHashMapUnmanaged([]const u8) {
        return p.parseTokens(&self.environ, self.alloc(), &self.args, &self.tokens, &self.logger);
    }
};

test "parseTokens sets a normalized key/value" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    try tp.addToken("KEY", .literal, "value");

    const envs = try tp.run();
    try expectEqual(@as(usize, 1), envs.count());
    try expectEqualStrings("value", envs.get("KEY").?);
}

test "parseTokens concatenates multiple value tokens into one value" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    var tok: token.Token = .{ .key = "MULTI", .file = "testp.env" };
    try tok.values.append(tp.alloc(), .{ .kind = .literal, .value = "12", .line = 1, .byte = 1 });
    try tok.values.append(tp.alloc(), .{ .kind = .literal, .value = "34", .line = 2, .byte = 1 });
    try tp.tokens.append(tp.alloc(), tok);

    const envs = try tp.run();
    try expectEqualStrings("1234", envs.get("MULTI").?);
}

test "parseTokens resolves interpolation from the environment" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    try tp.environ.put("HOME", "/home/test");
    try tp.addToken("DIR", .interpolated, "HOME");

    const envs = try tp.run();
    try expectEqualStrings("/home/test", envs.get("DIR").?);
}

test "parseTokens resolves interpolation from a previously parsed env" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    try tp.addToken("FOO", .literal, "bar");
    try tp.addToken("BAZ", .interpolated, "FOO");

    const envs = try tp.run();
    try expectEqualStrings("bar", envs.get("BAZ").?);
}

test "parseTokens skips an undefined interpolation but keeps valid keys" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    try tp.addToken("VALID", .literal, "yes");
    try tp.addToken("UNDEF", .interpolated, "NOPE");

    const envs = try tp.run();
    try expectEqual(@as(usize, 1), envs.count());
    try expectEqualStrings("yes", envs.get("VALID").?);
    try expect(envs.get("UNDEF") == null);
}

test "parseTokens debug listing follows the powershell format" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    tp.args.debug = true;
    tp.args.format = .powershell;
    try tp.addToken("KEY", .literal, "it's ok");

    _ = try tp.run();

    try expect(std.mem.indexOf(u8, tp.logger.buffered(), "$env:KEY = 'it''s ok'") != null);
}

test "parseTokens errors when nothing parses" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    try expectError(error.NoParsedEnvs, tp.run());
}

test "parseTokens passes when a required env is present" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    try tp.args.required.append(tp.alloc(), "REQUIRED");
    try tp.addToken("REQUIRED", .literal, "ok");

    const envs = try tp.run();
    try expectEqualStrings("ok", envs.get("REQUIRED").?);
}

test "parseTokens errors when a required env is missing" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    try tp.args.required.append(tp.alloc(), "REQUIRED");
    try tp.addToken("OTHER", .literal, "x");

    try expectError(error.MissingRequiredEnvs, tp.run());
}

test "parseTokens logs skipped comments in debug mode" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    tp.args.debug = true;
    try tp.addToken("KEY", .literal, "value");
    try tp.addToken(null, .commented, "# a comment");

    _ = try tp.run();

    const out = tp.output();
    try expect(indexOf(u8, out, "Parsed a comment") != null);
    try expect(indexOf(u8, out, "# a comment") != null);
    try expect(indexOf(u8, out, "testp.env:1:1") != null);
}
