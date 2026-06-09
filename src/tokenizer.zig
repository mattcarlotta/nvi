const std = @import("std");
const tty = @import("tty.zig");
const arg = @import("arg.zig");
const Io = std.Io;
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

pub const ValueKind = enum { normalized, partialed, commented, interpolated, multilined };

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

    fn clone(self: Token, alloc: std.mem.Allocator) !Token {
        var token: Token = .{
            .file = try alloc.dupe(u8, self.file),
            .key = if (self.key) |k| try alloc.dupe(u8, k) else null,
        };

        try token.values.ensureTotalCapacity(alloc, self.values.items.len);

        for (self.values.items) |v| token.values.appendAssumeCapacity(.{
            .kind = v.kind,
            .value = try alloc.dupe(u8, v.value),
            .line = v.line,
            .byte = v.byte,
        });

        return token;
    }
};

pub const Tokenizer = struct {
    alloc: std.mem.Allocator,
    logger: *Io.Writer,
    i: usize = 0,
    byte: usize = 0,
    line: usize = 0,
    file: ?[]const u8 = null,
    file_name: []const u8 = undefined,
    file_path: ?[]const u8 = null,
    tokens: std.ArrayList(Token) = .empty,

    fn nextLine(self: *Tokenizer) void {
        self.line += 1;
    }

    fn resetByte(self: *Tokenizer) void {
        self.byte = 1;
    }

    fn next_byte(self: *Tokenizer, offset: ?usize) void {
        self.byte += offset orelse 1;
    }

    fn skipByte(self: *Tokenizer, offset: ?usize) void {
        const o = offset orelse 1;
        self.next_byte(o);
        self.i += o;
    }

    fn peek(self: *Tokenizer, offset: ?usize) ?u8 {
        const file = self.file orelse return null;
        const index = self.i + (offset orelse 0);
        if (index >= file.len) return null;
        return file[index];
    }

    fn get_char(self: *Tokenizer, offset: ?usize) ?u8 {
        return self.peek(offset);
    }

    fn commitToken(self: *Tokenizer, token: *Token, kind: ValueKind, bytes: []const u8) !void {
        try token.values.append(self.alloc, .{
            .kind = kind,
            .value = try self.alloc.dupe(u8, bytes),
            .line = self.line,
            .byte = self.byte,
        });
    }

    fn parse(self: *Tokenizer) !void {
        var token: Token = .{ .file = try self.alloc.dupe(u8, self.file_name) };

        var value: std.ArrayList(u8) = .empty;
        defer value.deinit(self.alloc);

        while (self.peek(null)) |current_char| {
            switch (current_char) {
                NULL_CHAR => self.skipByte(null),
                LINE_DELIMITER => {
                    if (token.key != null) {
                        try self.commitToken(&token, ValueKind.normalized, value.items);
                        try self.tokens.append(self.alloc, try token.clone(self.alloc));
                    }
                    token.key = null;
                    token.values.clearRetainingCapacity();
                    value.clearRetainingCapacity();
                    self.nextLine();
                    self.skipByte(null);
                    self.resetByte();
                },
                ASSIGN_OP => {
                    token.key = try self.alloc.dupe(u8, value.items);
                    value.clearRetainingCapacity();
                    // skipByte '='
                    self.skipByte(null);
                },
                HASH => {
                    // hash inside a started key: treat as literal
                    if (token.key != null) {
                        try value.append(self.alloc, current_char);
                        self.skipByte(null);
                        continue;
                    }
                    // otherwise a commented: consume to end of line
                    while (self.peek(null) != null and (self.get_char(null) orelse 0) != LINE_DELIMITER) {
                        try value.append(self.alloc, self.get_char(null).?);
                        self.skipByte(null);
                    }
                    try self.commitToken(&token, ValueKind.commented, value.items);
                    try self.tokens.append(self.alloc, try token.clone(self.alloc));
                    value.clearRetainingCapacity();
                },
                DOLLAR_SIGN => {
                    // not "${": literal '$'
                    if ((self.get_char(1) orelse 0) != OPEN_BRACE) {
                        try value.append(self.alloc, current_char);
                        self.skipByte(null);
                        continue;
                    }

                    // commit anything accumulated before the "${"
                    if (value.items.len != 0) {
                        try self.commitToken(&token, ValueKind.partialed, value.items);
                        value.clearRetainingCapacity();
                    }

                    // skipByte "${"
                    self.skipByte(2);

                    // interpolate ENV within "${ENV}"
                    while (self.peek(null) != null) {
                        switch (self.get_char(null) orelse 0) {
                            CLOSE_BRACE => {
                                // skipByte '}'
                                self.skipByte(null);
                                break;
                            },
                            LINE_DELIMITER => {
                                try self.logger.print(
                                    tty.red ++ "error:" ++ tty.reset ++ " A tokenizing error occurred in " ++ tty.bold ++ tty.red ++ "{s}:{d}:{d}. " ++ tty.reset,
                                    .{ self.file_name, self.line, self.byte },
                                );

                                try self.logger.print("The " ++ tty.bold ++ tty.red ++ "{s}" ++ tty.reset ++ " key has an unterminated value interpolation.\n", .{token.key orelse ("none")});
                                try self.logger.print("     ${{{s}\n", .{value.items});
                                try self.logger.writeAll(tty.green ++ "      ^");
                                for (0..value.items.len) |_| try self.logger.writeByte('~');
                                try self.logger.writeAll(tty.reset ++ " (missing a closing brace '}')\n");
                                return error.MissingClosingBrace;
                            },
                            else => {
                                if (self.peek(null)) |c| try value.append(self.alloc, c);
                                self.skipByte(null);
                            },
                        }
                    }

                    try self.commitToken(&token, ValueKind.interpolated, value.items);

                    if ((self.get_char(null) orelse 0) == LINE_DELIMITER) {
                        try self.tokens.append(self.alloc, try token.clone(self.alloc));
                        token.key = null;
                        token.values.clearRetainingCapacity();
                        self.nextLine();
                        // skipByte '\n'
                        self.skipByte(null);
                        self.resetByte();
                    }
                    value.clearRetainingCapacity();
                },
                BACK_SLASH => {
                    // literal backslash: next char exists and isn't a newline
                    if (self.peek(1) != null and (self.get_char(1) orelse 0) != LINE_DELIMITER) {
                        try value.append(self.alloc, current_char);
                        self.skipByte(null);
                        continue;
                    }

                    if (value.items.len != 0) {
                        try self.commitToken(&token, ValueKind.normalized, value.items);
                    }

                    // skipByte "\\n"
                    self.skipByte(2);
                    self.resetByte();
                    value.clearRetainingCapacity();

                    // multilined values
                    while (self.peek(null) != null) {
                        const is_eol = (self.get_char(null) orelse 0) == LINE_DELIMITER;

                        if (((self.get_char(null) orelse 0) == BACK_SLASH and
                            (self.get_char(1) orelse 0) == LINE_DELIMITER) or is_eol)
                        {
                            self.nextLine();
                            try self.commitToken(&token, ValueKind.multilined, value.items);
                            value.clearRetainingCapacity();
                            self.resetByte();

                            // skipByte '\n' or "\\n"
                            const bytes: usize = if (is_eol) 1 else 2;
                            self.skipByte(bytes);

                            if (is_eol) break;
                        }

                        if (self.peek(null)) |c| try value.append(self.alloc, c);

                        self.skipByte(null);
                    }

                    try self.tokens.append(self.alloc, try token.clone(self.alloc));
                    token.key = null;
                    token.values.clearRetainingCapacity();
                    value.clearRetainingCapacity();
                    self.resetByte();
                    self.nextLine();
                },
                else => {
                    try value.append(self.alloc, current_char);
                    self.skipByte(null);
                },
            }
        }

        if (self.tokens.items.len == 0) {
            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " Unable to generate any tokens for " ++ tty.bold ++ tty.red ++ "{s}" ++ tty.reset ++ ". Aborting.\n", .{self.file_name});
            return error.NoTokensGenerated;
        }
    }
};

pub fn parseFiles(io: Io, alloc: std.mem.Allocator, args: *const arg.Arg, logger: *Io.Writer) !std.ArrayList(Token) {
    var tokenizer: Tokenizer = .{ .alloc = alloc, .logger = logger };

    for (args.files.items) |env| {
        tokenizer.i = 0;
        tokenizer.byte = 1;
        tokenizer.line = 1;
        tokenizer.file_name = env;

        const contents = Io.Dir.cwd().readFileAlloc(io, tokenizer.file_name, tokenizer.alloc, .limited(1024 * 1024)) catch {
            try tokenizer.logger.print(
                tty.red ++ "error:" ++ tty.reset ++ " Unable to locate a " ++ tty.bold ++ tty.red ++ "{s}" ++ tty.reset ++ " within the current directory (file not found).\n",
                .{tokenizer.file_name},
            );
            return error.FileReadFailed;
        };

        if (contents.len == 0) {
            try tokenizer.logger.print(
                tty.red ++ "error:" ++ tty.reset ++ " The " ++ tty.bold ++ tty.red ++ "{s}" ++ tty.reset ++ " file doesn't contain any environment variables!\n",
                .{tokenizer.file_name},
            );
            return error.EmptyFile;
        }

        tokenizer.file = contents;

        try tokenizer.parse();
    }

    if (args.debug) {
        try tokenizer.logger.print(tty.cyan ++ "\ninfo: " ++ tty.reset ++ "The following {d} token(s) have been generated...", .{tokenizer.tokens.items.len});

        for (tokenizer.tokens.items, 0..) |token, ti| {
            try tokenizer.logger.print(tty.cyan ++ "\n  token #{d}:" ++ tty.reset ++ "\n", .{ti + 1});
            try tokenizer.logger.print("    • file: {s}\n", .{token.file});
            try tokenizer.logger.print("    • key: " ++ tty.bold ++ tty.green ++ "{s}" ++ tty.reset ++ "\n", .{token.key orelse "(none)"});
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
        try tokenizer.logger.writeByte('\n');
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

    fn generate(self: *TestTokenizer, content: []const u8) !std.ArrayList(Token) {
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

    const tokens = try t.generate("KEY=value\n");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expectEqualStrings("KEY", tokens.items[0].key.?);
    try expectEqual(@as(usize, 1), tokens.items[0].values.items.len);
    try expectEqual(ValueKind.normalized, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("value", tokens.items[0].values.items[0].value);
}

test "parses multiple line key/values" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.generate("A=123\\\n456\n");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expectEqualStrings("A", tokens.items[0].key.?);
    try expectEqual(@as(usize, 2), tokens.items[0].values.items.len);
    try expectEqual(ValueKind.normalized, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("123", tokens.items[0].values.items[0].value);
    try expectEqual(ValueKind.multilined, tokens.items[0].values.items[1].kind);
    try expectEqualStrings("456", tokens.items[0].values.items[1].value);
}

test "parses a comment" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.generate("# a comment\n");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expect(tokens.items[0].key == null);
    try expectEqual(ValueKind.commented, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("# a comment", tokens.items[0].values.items[0].value);
}

test "parses an interpolated value" {
    var t = TestTokenizer.init();
    defer t.deinit();

    const tokens = try t.generate("KEY=${OTHER}\n");
    try expectEqual(@as(usize, 1), tokens.items.len);
    try expectEqualStrings("KEY", tokens.items[0].key.?);
    try expectEqual(ValueKind.interpolated, tokens.items[0].values.items[0].kind);
    try expectEqualStrings("OTHER", tokens.items[0].values.items[0].value);
}

test "errors on unterminated interpolation" {
    var t = TestTokenizer.init();
    defer t.deinit();

    try expectError(error.MissingClosingBrace, t.generate("KEY=${OTHER\n"));
}

test "errors when no tokens are generated" {
    var t = TestTokenizer.init();
    defer t.deinit();

    try expectError(error.NoTokensGenerated, t.generate("novalue\n"));
}
