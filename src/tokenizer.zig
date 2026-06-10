const std = @import("std");
const tty = @import("tty.zig");
const arg = @import("arg.zig");
const Io = std.Io;
const mem = std.mem;
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;
const expectError = std.testing.expectError;

const NULL_CHAR: u8 = 0;
const LINE_DELIMITER: u8 = '\n';
const ASSIGN_OP: u8 = '=';
const HASH: u8 = '#';
const DOLLAR_SIGN: u8 = '$';
const OPEN_BRACE: u8 = '{';
const CLOSE_BRACE: u8 = '}';
const BACK_SLASH: u8 = '\\';

// every character the main loop treats specially; used to scan plain runs in one pass
const SPECIAL_CHARS = [_]u8{ NULL_CHAR, LINE_DELIMITER, ASSIGN_OP, HASH, DOLLAR_SIGN, BACK_SLASH };

pub const ValueKind = enum { literal, commented, interpolated };

const ValueToken = struct {
    kind: ValueKind,
    value: []const u8,
    line: usize,
    byte: usize,
};

pub const Token = struct {
    key: ?[]const u8 = null,
    file: []const u8 = "",
    values: std.ArrayList(ValueToken) = .empty,
};

pub const Tokenizer = struct {
    alloc: mem.Allocator,
    logger: *Io.Writer,
    i: usize = 0,
    byte: usize = 0,
    line: usize = 0,
    file: ?[]const u8 = null,
    file_name: []const u8 = undefined,
    tokens: std.ArrayList(Token) = .empty,

    fn nextLine(self: *Tokenizer) void {
        self.line += 1;
    }

    fn resetByte(self: *Tokenizer) void {
        self.byte = 1;
    }

    fn next_byte(self: *Tokenizer, offset: usize) void {
        self.byte += offset;
    }

    fn skipByte(self: *Tokenizer, offset: usize) void {
        self.next_byte(offset);
        self.i += offset;
    }

    fn peek(self: *Tokenizer, offset: usize) ?u8 {
        const file = self.file orelse return null;
        const index = self.i + offset;
        if (index >= file.len) return null;
        return file[index];
    }

    fn scanUntil(self: *Tokenizer, value: *std.ArrayList(u8), chars: []const u8) !void {
        const end = mem.indexOfAnyPos(u8, self.file.?, self.i, chars) orelse self.file.?.len;
        try value.appendSlice(self.alloc, self.file.?[self.i..end]);
        self.next_byte(end - self.i);
        self.i = end;
    }

    fn commitToken(self: *Tokenizer, token: *Token, kind: ValueKind, bytes: []const u8) !void {
        try token.values.append(self.alloc, .{
            .kind = kind,
            .value = try self.alloc.dupe(u8, bytes),
            .line = self.line,
            .byte = self.byte,
        });
    }

    fn appendToken(self: *Tokenizer, token: *Token) !void {
        try self.tokens.append(self.alloc, token.*);
        token.* = .{ .file = self.file_name };
    }

    fn printErrorHeader(self: *Tokenizer) !void {
        try self.logger.print(
            tty.red ++ "error:" ++ tty.reset ++ " A tokenizing error occurred in " ++ tty.bold_red ++ "{s}:{d}:{d}" ++ tty.reset ++ ". ",
            .{ self.file_name, self.line, self.byte },
        );
    }

    fn printTokenLine(self: *Tokenizer, token: *const Token) !usize {
        const key: []const u8 = token.key orelse "(none)";
        try self.logger.print("   {s}=", .{key});

        var prefix_len: usize = key.len + 1;
        for (token.values.items) |v| {
            if (v.kind == .literal) {
                try self.logger.writeAll(v.value);
                prefix_len += v.value.len;
            }
        }
        return prefix_len;
    }

    fn parse(self: *Tokenizer) !void {
        var token: Token = .{ .file = self.file_name };

        var value: std.ArrayList(u8) = .empty;
        defer value.deinit(self.alloc);

        while (self.peek(0)) |current_char| {
            switch (current_char) {
                NULL_CHAR => self.skipByte(1),
                LINE_DELIMITER => {
                    if (token.key != null) {
                        try self.commitToken(&token, ValueKind.literal, value.items);
                        try self.appendToken(&token);
                    }
                    value.clearRetainingCapacity();
                    self.nextLine();
                    self.skipByte(1);
                    self.resetByte();
                },
                ASSIGN_OP => {
                    // '=' inside a value is literal
                    if (token.key != null) {
                        try value.append(self.alloc, current_char);
                        self.skipByte(1);
                        continue;
                    }

                    const trimmed_key = mem.trim(u8, value.items, " \t");
                    if (trimmed_key.len == 0) {
                        try self.printErrorHeader();
                        try self.logger.writeAll("A value assignment ('=') was found without a key name.\n");

                        const end = mem.indexOfScalarPos(u8, self.file.?, self.i, LINE_DELIMITER) orelse self.file.?.len;
                        const rest = self.file.?[self.i..end];

                        try self.logger.print("   {s}\n", .{rest});

                        try self.logger.writeAll("   " ++ tty.green ++ "^");
                        if (rest.len > 1) try self.logger.splatByteAll('~', rest.len - 1);
                        try self.logger.writeAll(" (missing key)\n" ++ tty.reset);

                        return error.EmptyKey;
                    }

                    token.key = try self.alloc.dupe(u8, trimmed_key);
                    value.clearRetainingCapacity();
                    // skipByte '='
                    self.skipByte(1);
                },
                HASH => {
                    // hash inside a literal
                    if (token.key != null) {
                        try value.append(self.alloc, current_char);
                        self.skipByte(1);
                        continue;
                    }

                    try self.scanUntil(&value, "\n");

                    try self.commitToken(&token, ValueKind.commented, value.items);
                    try self.appendToken(&token);
                    value.clearRetainingCapacity();
                },
                DOLLAR_SIGN => {
                    if ((self.peek(1) orelse 0) != OPEN_BRACE) {
                        try value.append(self.alloc, current_char);
                        self.skipByte(1);
                        continue;
                    }

                    // commit anything accumulated before the "${"
                    if (value.items.len != 0) {
                        try self.commitToken(&token, ValueKind.literal, value.items);
                        value.clearRetainingCapacity();
                    }

                    // skipByte "${"
                    self.skipByte(2);

                    try self.scanUntil(&value, "}\n");

                    if ((self.peek(0) orelse LINE_DELIMITER) != CLOSE_BRACE) {
                        try self.printErrorHeader();
                        try self.logger.print(
                            "The " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " key has an unterminated value interpolation.\n",
                            .{token.key orelse "(none)"},
                        );

                        const prefix_len = try self.printTokenLine(&token);
                        try self.logger.print("${{{s}\n", .{value.items});

                        try self.logger.writeAll("   " ++ tty.green);
                        try self.logger.splatByteAll(' ', prefix_len + 1);
                        try self.logger.writeByte('^');
                        try self.logger.splatByteAll('~', value.items.len);
                        try self.logger.writeAll(" (missing a closing brace '}')\n" ++ tty.reset);
                        return error.MissingClosingBrace;
                    }

                    // skipByte '}'
                    self.skipByte(1);

                    if (value.items.len == 0) {
                        try self.printErrorHeader();
                        try self.logger.print(
                            "The " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " key has an invalid key interpolation.\n",
                            .{token.key orelse "(none)"},
                        );

                        const prefix_len = try self.printTokenLine(&token);
                        try self.logger.writeAll("${}\n");

                        try self.logger.writeAll("   " ++ tty.green);
                        try self.logger.splatByteAll(' ', prefix_len + 1);
                        try self.logger.writeAll("^~ (unresolvable interpolation key)\n" ++ tty.reset);

                        return error.EmptyInterpolationKey;
                    }

                    try self.commitToken(&token, ValueKind.interpolated, value.items);

                    if ((self.peek(0) orelse 0) == LINE_DELIMITER) {
                        try self.appendToken(&token);
                        self.nextLine();
                        // skipByte '\n'
                        self.skipByte(1);
                        self.resetByte();
                    }
                    value.clearRetainingCapacity();
                },
                BACK_SLASH => {
                    if (self.peek(1) != null and (self.peek(1) orelse 0) != LINE_DELIMITER) {
                        try value.append(self.alloc, current_char);
                        self.skipByte(1);
                        continue;
                    }

                    // line continuation: commit the current segment and keep
                    // tokenizing the same token from the main loop, so '$',
                    // '#', and '=' on continuation lines are handled normally
                    if (value.items.len != 0) {
                        try self.commitToken(&token, ValueKind.literal, value.items);
                    }

                    value.clearRetainingCapacity();
                    // skipByte "\\n"
                    self.skipByte(2);
                    self.nextLine();
                    self.resetByte();
                },
                else => {
                    try self.scanUntil(&value, &SPECIAL_CHARS);
                },
            }
        }

        // flush a pending token if the file doesn't end with a newline
        if (token.key != null) {
            if (value.items.len > 0 or token.values.items.len == 0) {
                try self.commitToken(&token, ValueKind.literal, value.items);
            }
            try self.appendToken(&token);
        }

        if (self.tokens.items.len == 0) {
            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " Unable to run any tokens for " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ ". Aborting.\n", .{self.file_name});
            return error.NoTokensGenerated;
        }
    }
};

pub fn parseFiles(io: Io, alloc: mem.Allocator, args: *const arg.Arg, logger: *Io.Writer) !std.ArrayList(Token) {
    var tokenizer: Tokenizer = .{ .alloc = alloc, .logger = logger };

    for (args.files.items) |env| {
        tokenizer.i = 0;
        tokenizer.byte = 1;
        tokenizer.line = 1;
        tokenizer.file_name = env;

        const contents = Io.Dir.cwd().readFileAlloc(io, tokenizer.file_name, tokenizer.alloc, .limited(1024 * 1024)) catch {
            try tokenizer.logger.print(
                tty.red ++ "error:" ++ tty.reset ++ " Unable to locate a " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " within the current directory (file not found).\n",
                .{tokenizer.file_name},
            );
            return error.FileReadFailed;
        };

        if (contents.len == 0) {
            try tokenizer.logger.print(
                tty.red ++ "error:" ++ tty.reset ++ " The " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " file doesn't contain any environment variables!\n",
                .{tokenizer.file_name},
            );
            return error.EmptyEnvFile;
        }

        tokenizer.file = contents;

        try tokenizer.tokens.ensureTotalCapacity(
            alloc,
            tokenizer.tokens.items.len + mem.count(u8, contents, "\n") + 1,
        );

        try tokenizer.parse();
    }

    if (args.debug) {
        try tokenizer.logger.print(tty.cyan ++ "info: " ++ tty.reset ++ "The following {d} token(s) have been created...", .{tokenizer.tokens.items.len});

        for (tokenizer.tokens.items, 0..) |token, ti| {
            try tokenizer.logger.print(tty.cyan ++ "\n  token #{d}:" ++ tty.reset ++ "\n", .{ti + 1});
            try tokenizer.logger.print("    • file: {s}\n", .{token.file});
            try tokenizer.logger.print("    • key: " ++ tty.bold_green ++ "{s}" ++ tty.reset ++ "\n", .{token.key orelse "(none)"});
            try tokenizer.logger.print("    • values ({d}):", .{token.values.items.len});

            for (token.values.items) |value| {
                try tokenizer.logger.print(
                    "\n     • {s} value -> " ++ tty.green ++ "{s}" ++ tty.reset ++ " (line {d}, byte {d})",
                    .{
                        @tagName(value.kind),
                        value.value,
                        value.line,
                        value.byte,
                    },
                );
            }
        }

        try tokenizer.logger.writeAll("\n\n");
    }

    return tokenizer.tokens;
}

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

    fn run(self: *TestTokenizer, content: []const u8) !std.ArrayList(Token) {
        self.logger = .fixed(&self.logger_buf);

        var tokenizer: Tokenizer = .{
            .alloc = self.arena.allocator(),
            .logger = &self.logger,
            .i = 0,
            .byte = 1,
            .line = 1,
            .file_name = "test.env",
            .file = content,
        };

        try tokenizer.parse();

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
    try expectEqual(ValueKind.literal, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("value", tokens.items[0].values.items[0].value);
}

test "parses a key/value without a trailing newline" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.run("KEY=value");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expectEqualStrings("KEY", tokens.items[0].key.?);
    try expectEqual(ValueKind.literal, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("value", tokens.items[0].values.items[0].value);
}

test "parses multiple line key/values" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.run("A=123\\\n456\n");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expectEqualStrings("A", tokens.items[0].key.?);
    try expectEqual(@as(usize, 2), tokens.items[0].values.items.len);
    try expectEqual(ValueKind.literal, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("123", tokens.items[0].values.items[0].value);
    try expectEqual(ValueKind.literal, tokens.items[0].values.items[1].kind);
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
    try expectEqual(ValueKind.literal, values[0].kind);
    try expectEqualStrings("abc", values[0].value);
    try expectEqual(ValueKind.interpolated, values[1].kind);
    try expectEqualStrings("OTHER", values[1].value);
    try expectEqual(ValueKind.literal, values[2].kind);
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
    try expectEqual(ValueKind.literal, values[0].kind);
    try expectEqualStrings("ssh-rsa ABC", values[0].value);
    try expectEqual(ValueKind.literal, values[1].kind);
    try expectEqualStrings("g3HI$", values[1].value);
    try expectEqual(ValueKind.literal, values[2].kind);
    try expectEqualStrings("+jk", values[2].value);
    try expectEqual(ValueKind.interpolated, values[3].kind);
    try expectEqualStrings("MESSAGE", values[3].value);
    try expectEqual(ValueKind.literal, values[4].kind);
    try expectEqualStrings("/4", values[4].value);
    try expectEqual(ValueKind.literal, values[5].kind);
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
    try expectEqual(ValueKind.commented, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("# a comment", tokens.items[0].values.items[0].value);
}

test "parses an interpolated value" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.run("KEY=${OTHER}\n");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expectEqualStrings("KEY", tokens.items[0].key.?);
    try expectEqual(ValueKind.interpolated, tokens.items[0].values.items[0].kind);
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
    try expect(mem.indexOf(u8, out, "key has an invalid key interpolation") != null);
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
