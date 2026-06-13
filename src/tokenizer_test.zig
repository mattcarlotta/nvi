const std = @import("std");
const tk = @import("tokenizer.zig");

const Io = std.Io;
const mem = std.mem;
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;
const expectError = std.testing.expectError;

const TestTokenizer = struct {
    arena: std.heap.ArenaAllocator,
    logger_buf: [4096]u8 = undefined,
    logger: Io.Writer = undefined,

    fn init() TestTokenizer {
        return .{ .arena = std.heap.ArenaAllocator.init(std.testing.allocator) };
    }

    fn deinit(self: *TestTokenizer) void {
        self.arena.deinit();
    }

    fn output(self: *TestTokenizer) []const u8 {
        return self.logger.buffered();
    }

    fn run(self: *TestTokenizer, file: []const u8) !std.ArrayList(tk.Token) {
        self.logger = .fixed(&self.logger_buf);

        var tokenizer: tk.Tokenizer = .{
            .alloc = self.arena.allocator(),
            .logger = &self.logger,
        };

        try tokenizer.parse(file, "test.env");

        return tokenizer.tokens;
    }
};

test "parses a simple key/value" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.run("KEY=value\n");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expectEqualStrings("KEY", tokens.items[0].key.?);
    try expectEqual(@as(usize, 1), tokens.items[0].values.items.len);
    try expectEqual(tk.ValueKind.literal, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("value", tokens.items[0].values.items[0].value);
}

test "parses a key/value without a trailing newline" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.run("KEY=value");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expectEqualStrings("KEY", tokens.items[0].key.?);
    try expectEqual(tk.ValueKind.literal, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("value", tokens.items[0].values.items[0].value);
}

test "parses multiple line key/values" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.run("A=123\\\n456\n");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expectEqualStrings("A", tokens.items[0].key.?);
    try expectEqual(@as(usize, 2), tokens.items[0].values.items.len);
    try expectEqual(tk.ValueKind.literal, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("123", tokens.items[0].values.items[0].value);
    try expectEqual(tk.ValueKind.literal, tokens.items[0].values.items[1].kind);
    try expectEqualStrings("456", tokens.items[0].values.items[1].value);
    try expectEqual(@as(usize, 1), tokens.items[0].values.items[0].line);
    try expectEqual(@as(usize, 2), tokens.items[0].values.items[1].line);
}

test "parses an interpolated value inside a multiline value" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.run("KEY=abc\\\n${OTHER}def\n");
    try expectEqual(@as(usize, 1), tokens.items.len);

    const values = tokens.items[0].values.items;
    try expectEqual(@as(usize, 3), values.len);
    try expectEqual(tk.ValueKind.literal, values[0].kind);
    try expectEqualStrings("abc", values[0].value);
    try expectEqual(tk.ValueKind.interpolated, values[1].kind);
    try expectEqualStrings("OTHER", values[1].value);
    try expectEqual(tk.ValueKind.literal, values[2].kind);
    try expectEqualStrings("def", values[2].value);
}

test "parses a multiline ssh key with interpolation and literal '=' and '$'" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.run("MULTI=ssh-rsa ABC\\\ng3HI$\\\n+jk${MESSAGE}/4\\\nLm5Mn== test@example.com\n");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expectEqualStrings("MULTI", tokens.items[0].key.?);

    const values = tokens.items[0].values.items;
    try expectEqual(@as(usize, 6), values.len);
    try expectEqual(tk.ValueKind.literal, values[0].kind);
    try expectEqualStrings("ssh-rsa ABC", values[0].value);
    try expectEqual(tk.ValueKind.literal, values[1].kind);
    try expectEqualStrings("g3HI$", values[1].value);
    try expectEqual(tk.ValueKind.literal, values[2].kind);
    try expectEqualStrings("+jk", values[2].value);
    try expectEqual(tk.ValueKind.interpolated, values[3].kind);
    try expectEqualStrings("MESSAGE", values[3].value);
    try expectEqual(tk.ValueKind.literal, values[4].kind);
    try expectEqualStrings("/4", values[4].value);
    try expectEqual(tk.ValueKind.literal, values[5].kind);
    try expectEqualStrings("Lm5Mn== test@example.com", values[5].value);
}

test "treats '=' as literal after the key is set" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.run("KEY=a==b\n");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expectEqualStrings("KEY", tokens.items[0].key.?);
    try expectEqualStrings("a==b", tokens.items[0].values.items[0].value);
}

test "parses a comment" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.run("# a comment\n");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expect(tokens.items[0].key == null);
    try expectEqual(tk.ValueKind.commented, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("# a comment", tokens.items[0].values.items[0].value);
}

test "parses an interpolated value" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.run("KEY=${OTHER}\n");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expectEqualStrings("KEY", tokens.items[0].key.?);
    try expectEqual(tk.ValueKind.interpolated, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("OTHER", tokens.items[0].values.items[0].value);
}

test "errors on unterminated interpolation" {
    var t = TestTokenizer.init();
    defer t.deinit();

    try expectError(error.MissingClosingBrace, t.run("KEY=${OTHER\n"));
}

test "errors on unterminated interpolation at end of file" {
    var t = TestTokenizer.init();
    defer t.deinit();

    try expectError(error.MissingClosingBrace, t.run("KEY=${OTHER"));
}

test "errors when no tokens are rund" {
    var t = TestTokenizer.init();
    defer t.deinit();

    try expectError(error.NoTokensGenerated, t.run("novalue\n"));
}

test "errors on empty key (=value)" {
    var t = TestTokenizer.init();
    defer t.deinit();

    try expectError(error.EmptyKey, t.run("=abc123\n"));
    const out = t.output();
    try expect(mem.indexOf(u8, out, "was found without a key name") != null);
    try expect(mem.indexOf(u8, out, "=abc123") != null);
}

test "errors on whitespace-only key" {
    var t = TestTokenizer.init();
    defer t.deinit();

    try expectError(error.EmptyKey, t.run("   =value\n"));
    try expect(mem.indexOf(u8, t.output(), "was found without a key name") != null);
}

test "errors on empty interpolation key (${})" {
    var t = TestTokenizer.init();
    defer t.deinit();

    try expectError(error.EmptyInterpolationKey, t.run("KEY=abc${}123\n"));
    const out = t.output();
    try expect(mem.indexOf(u8, out, "key has an undefined key interpolation") != null);
    try expect(mem.indexOf(u8, out, "KEY=abc${}") != null);
}

test "errors on unterminated interpolation (${ with no close)" {
    var t = TestTokenizer.init();
    defer t.deinit();

    try expectError(error.MissingClosingBrace, t.run("KEY=abc${XY\n"));
    const out = t.output();
    try expect(mem.indexOf(u8, out, "key has an unterminated value interpolation") != null);
    try expect(mem.indexOf(u8, out, "${XY") != null);
}
