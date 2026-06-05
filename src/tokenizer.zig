const std = @import("std");
const Io = std.Io;

const NULL_CHAR: u8 = 0;
const LINE_DELIMITER: u8 = '\n';
const ASSIGN_OP: u8 = '=';
const HASH: u8 = '#';
const DOLLAR_SIGN: u8 = '$';
const OPEN_BRACE: u8 = '{';
const CLOSE_BRACE: u8 = '}';
const BACK_SLASH: u8 = '\\';

pub const Value = enum { normal, comment, interpolated, multiline };

pub const ValueToken = struct {
    kind: Value,
    value: []const u8,
    line: usize,
    byte: usize,
};

pub const Token = struct {
    key: ?[]const u8 = null,
    file: []const u8 = "",
    values: std.ArrayList(ValueToken) = .empty,

    fn clone(self: Token, alloc: std.mem.Allocator) !Token {
        var copy: Token = .{
            .file = try alloc.dupe(u8, self.file),
            .key = if (self.key) |k| try alloc.dupe(u8, k) else null,
        };

        try copy.values.ensureTotalCapacity(alloc, self.values.items.len);

        for (self.values.items) |v| copy.values.appendAssumeCapacity(.{
            .kind = v.kind,
            .value = try alloc.dupe(u8, v.value),
            .line = v.line,
            .byte = v.byte,
        });

        return copy;
    }
};

pub const TokenizeError = error{
    MissingClosingBrace,
    NoTokensGenerated,
} || std.mem.Allocator.Error || Io.Writer.Error;

const Tokenizer = struct {
    alloc: std.mem.Allocator,
    logger: *Io.Writer,
    i: usize = 0,
    byte: usize = 0,
    line: usize = 0,
    file: []const u8,
    file_name: []const u8,
    file_path: []const u8,
    tokens: std.ArrayList(Token) = .empty,

    fn inc_line(self: *Tokenizer) void {
        self.line += 1;
    }

    fn reset_byte(self: *Tokenizer) void {
        self.byte = 1;
    }

    fn inc_byte(self: *Tokenizer, offset: ?usize) void {
        self.byte += offset orelse 1;
    }

    fn peek(self: *Tokenizer, offset: ?usize) ?u8 {
        const index = self.i + (offset orelse 0);
        if (index >= self.file.len) return null;
        return self.file[index];
    }

    fn get_char(self: *Tokenizer, offset: ?usize) ?u8 {
        return self.peek(offset);
    }

    fn skip(self: *Tokenizer, offset: ?usize) void {
        const o = offset orelse 1;
        self.inc_byte(o);
        self.i += o;
    }

    fn pushValue(self: *Tokenizer, token: *Token, kind: Value, bytes: []const u8) !void {
        try token.values.append(self.alloc, .{
            .kind = kind,
            .value = try self.alloc.dupe(u8, bytes),
            .line = self.line,
            .byte = self.byte,
        });
    }

    fn parse(self: *Tokenizer) TokenizeError!void {
        var token: Token = .{ .file = try self.alloc.dupe(u8, self.file_name) };

        var value: std.ArrayList(u8) = .empty;
        defer value.deinit(self.alloc);

        while (self.peek(null)) |current_char| {
            switch (current_char) {
                NULL_CHAR => self.skip(null),
                LINE_DELIMITER => {
                    if (token.key != null) {
                        try self.pushValue(&token, .normal, value.items);
                        try self.tokens.append(self.alloc, try token.clone(self.alloc));
                    }
                    token.key = null;
                    token.values.clearRetainingCapacity();
                    value.clearRetainingCapacity();
                    self.inc_line();
                    self.skip(null);
                    self.reset_byte();
                },
                ASSIGN_OP => {
                    token.key = try self.alloc.dupe(u8, value.items);
                    value.clearRetainingCapacity();
                    self.skip(null); // skip '='
                },
                HASH => {
                    // hash inside a started key: treat as literal
                    if (token.key != null) {
                        try value.append(self.alloc, current_char);
                        self.skip(null);
                        continue;
                    }
                    // otherwise a comment: consume to end of line
                    while (self.peek(null) != null and (self.get_char(null) orelse 0) != LINE_DELIMITER) {
                        try value.append(self.alloc, self.get_char(null).?);
                        self.skip(null);
                    }
                    try self.pushValue(&token, .comment, value.items);
                    try self.tokens.append(self.alloc, try token.clone(self.alloc));
                    value.clearRetainingCapacity();
                },
                DOLLAR_SIGN => {
                    // not "${": literal '$'
                    if ((self.get_char(1) orelse 0) != OPEN_BRACE) {
                        try value.append(self.alloc, current_char);
                        self.skip(null);
                        continue;
                    }
                    // commit anything accumulated before the "${"
                    if (value.items.len != 0) {
                        try self.pushValue(&token, .normal, value.items);
                        value.clearRetainingCapacity();
                    }
                    self.skip(2); // skip "${"

                    // extract ENV within "${ENV}"
                    while (self.peek(null) != null) {
                        switch (self.get_char(null) orelse 0) {
                            CLOSE_BRACE => {
                                self.skip(null); // skip '}'
                                break;
                            },
                            LINE_DELIMITER => {
                                try self.logger.print(
                                    "error in {s}:{d}:{d}: key '{s}' opens an interpolation with \"${{\" but never closes it with \"}}\".\n",
                                    .{ self.file_name, self.line, self.byte, token.key orelse "" },
                                );
                                return error.MissingClosingBrace;
                            },
                            else => {
                                if (self.peek(null)) |c| try value.append(self.alloc, c);
                                self.skip(null);
                            },
                        }
                    }

                    try self.pushValue(&token, .interpolated, value.items);

                    if ((self.get_char(null) orelse 0) == LINE_DELIMITER) {
                        try self.tokens.append(self.alloc, try token.clone(self.alloc));
                        token.key = null;
                        token.values.clearRetainingCapacity();
                        self.inc_line();
                        // skip '\n'
                        self.skip(null);
                        self.reset_byte();
                    }
                    value.clearRetainingCapacity();
                },

                BACK_SLASH => {
                    // literal backslash: next char exists and isn't a newline
                    if (self.peek(1) != null and (self.get_char(1) orelse 0) != LINE_DELIMITER) {
                        try value.append(self.alloc, current_char);
                        self.skip(null);
                        continue;
                    }

                    if (value.items.len != 0) {
                        try self.pushValue(&token, .normal, value.items);
                    }

                    // skip "\\n"
                    self.skip(2);
                    self.reset_byte();
                    value.clearRetainingCapacity();

                    // multiline values
                    while (self.peek(null) != null) {
                        const is_eol = (self.get_char(null) orelse 0) == LINE_DELIMITER;
                        if (((self.get_char(null) orelse 0) == BACK_SLASH and
                            (self.get_char(1) orelse 0) == LINE_DELIMITER) or is_eol)
                        {
                            self.inc_line();
                            try self.pushValue(&token, .multiline, value.items);
                            value.clearRetainingCapacity();
                            self.reset_byte();

                            var next_byte: usize = 1;
                            if (!is_eol) next_byte += 1;

                            // skip '\n' or "\\n"
                            self.skip(next_byte);

                            if (is_eol) break;
                        }
                        if (self.peek(null)) |c| try value.append(self.alloc, c);
                        self.skip(null);
                    }

                    try self.tokens.append(self.alloc, try token.clone(self.alloc));
                    token.key = null;
                    token.values.clearRetainingCapacity();
                    value.clearRetainingCapacity();
                    self.reset_byte();
                    self.inc_line();
                },

                else => {
                    try value.append(self.alloc, current_char);
                    self.skip(null);
                },
            }
        }

        if (self.tokens.items.len == 0) {
            try self.logger.print("unable to generate any tokens for '{s}'. Aborting.\n", .{self.file_name});
            return error.NoTokensGenerated;
        }
    }
};
