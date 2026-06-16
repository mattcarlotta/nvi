const std = @import("std");
const Io = std.Io;
const mem = std.mem;

const arg = @import("arg.zig");
const char = @import("char.zig");
const tty = @import("tty.zig");

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
    io: Io,
    alloc: mem.Allocator,
    logger: *Io.Writer,
    args: *const arg.Arg,
    i: usize = 0,
    byte: usize = 0,
    line: usize = 0,
    file: []const u8 = "",
    file_name: []const u8 = "",
    tokens: std.ArrayList(Token) = .empty,

    pub fn run(self: *Tokenizer) !void {
        for (self.args.files.items) |env| {
            const file = Io.Dir.cwd().readFileAlloc(self.io, env, self.alloc, .limited(10 * 1024 * 1024)) catch {
                try self.logger.print(
                    tty.red ++ "error:" ++ tty.reset ++ " Unable to locate a(n) " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " file within the current directory (file not found).\n",
                    .{env},
                );
                return error.FileReadFailed;
            };

            if (file.len == 0) {
                try self.logger.print(
                    tty.red ++ "error:" ++ tty.reset ++ " Unable to generate tokens. The " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " file doesn't contain any KEY=VALUE variables!\n",
                    .{env},
                );
                return error.EmptyEnvFile;
            }

            try self.tokens.ensureTotalCapacity(
                self.alloc,
                self.tokens.items.len + mem.count(u8, file, "\n") + 1,
            );

            try self.parse(file, env);
        }

        if (self.args.dry_run) {
            try self.print();
        }
    }

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
        const index = self.i + offset;
        if (index >= self.file.len) return null;
        return self.file[index];
    }

    fn scanUntil(self: *Tokenizer, value: *std.ArrayList(u8), stops: []const u8) !void {
        const end = mem.indexOfAnyPos(u8, self.file, self.i, stops) orelse self.file.len;
        try value.appendSlice(self.alloc, self.file[self.i..end]);
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

    fn print(self: *Tokenizer) !void {
        try self.logger.print(tty.cyan ++ "info: " ++ tty.reset ++ "The following {d} token(s) have been created...", .{self.tokens.items.len});

        for (self.tokens.items, 0..) |token, ti| {
            try self.logger.print(tty.cyan ++ "\n  token #{d}:" ++ tty.reset ++ "\n", .{ti + 1});
            try self.logger.print("    • file: {s}\n", .{token.file});
            try self.logger.print("    • key: " ++ tty.bold_green ++ "{s}" ++ tty.reset ++ "\n", .{token.key orelse "(none)"});
            try self.logger.print("    • values ({d}):", .{token.values.items.len});

            for (token.values.items) |value| {
                try self.logger.print(
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

        try self.logger.writeAll("\n\n");
    }

    pub fn parse(self: *Tokenizer, file: []const u8, file_name: []const u8) !void {
        self.file_name = file_name;
        self.file = file;
        self.i = 0;
        self.byte = 1;
        self.line = 1;
        var token: Token = .{ .file = self.file_name };

        var value: std.ArrayList(u8) = .empty;
        defer value.deinit(self.alloc);

        while (self.peek(0)) |current_char| {
            switch (current_char) {
                char.NULL_CHAR => self.skipByte(1),
                char.LINE_DELIMITER => {
                    if (token.key != null) {
                        try self.commitToken(&token, ValueKind.literal, value.items);
                        try self.appendToken(&token);
                    }
                    value.clearRetainingCapacity();
                    self.nextLine();
                    self.skipByte(1);
                    self.resetByte();
                },
                char.ASSIGN_OP => {
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

                        const end = mem.indexOfScalarPos(u8, self.file, self.i, char.LINE_DELIMITER) orelse self.file.len;
                        const rest = self.file[self.i..end];

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
                char.HASH => {
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
                char.DOLLAR_SIGN => {
                    // dollar sign inside a literal
                    if ((self.peek(1) orelse 0) != char.OPEN_BRACE) {
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

                    if ((self.peek(0) orelse char.LINE_DELIMITER) != char.CLOSE_BRACE) {
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
                            "The " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " key has an undefined key interpolation.\n",
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

                    if ((self.peek(0) orelse 0) == char.LINE_DELIMITER) {
                        try self.appendToken(&token);
                        self.nextLine();
                        // skipByte '\n'
                        self.skipByte(1);
                        self.resetByte();
                    }
                    value.clearRetainingCapacity();
                },
                char.BACK_SLASH => {
                    if (self.peek(1) != null and (self.peek(1) orelse 0) != char.LINE_DELIMITER) {
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
                    try self.scanUntil(&value, &char.SPECIAL_CHARS);
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
            try self.logger.print(tty.red ++ "error:" ++ tty.reset ++ " Unable to generate tokens for " ++ tty.bold_red ++ "{s}" ++ tty.reset, .{self.file_name});
            try self.logger.writeAll(". Ensure the .env file is valid by following the KEY=VALUE spec. Aborting.\n");
            return error.NoTokensGenerated;
        }
    }
};

pub fn tokenFiles(io: Io, alloc: mem.Allocator, args: *const arg.Arg, logger: *Io.Writer) !std.ArrayList(Token) {
    var tokenizer: Tokenizer = .{ .args = args, .alloc = alloc, .io = io, .logger = logger };

    try tokenizer.run();

    return tokenizer.tokens;
}
