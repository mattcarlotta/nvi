const std = @import("std");
const tty = @import("tty.zig");
const arg = @import("arg.zig");
const token = @import("tokenizer.zig");
const Io = std.Io;
const Environ = std.process.Environ;
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;
const expectError = std.testing.expectError;

pub fn parseTokens(
    environ: *Environ.Map,
    alloc: std.mem.Allocator,
    args: *const arg.Arg,
    tokens: *const std.ArrayList(token.Token),
    logger: *Io.Writer,
) !std.StringArrayHashMapUnmanaged([]const u8) {
    var envs: std.StringArrayHashMapUnmanaged([]const u8) = .empty;

    for (tokens.items) |tkn| {
        if (tkn.values.items.len == 0 or tkn.key == null) continue;

        var value: std.ArrayList(u8) = .empty;
        defer value.deinit(alloc);

        for (tkn.values.items) |value_token| {
            if (value_token.value.len == 0) continue;

            switch (value_token.kind) {
                .interpolated => {
                    const interpolated_key = value_token.value;

                    if (environ.get(interpolated_key)) |v| {
                        try value.appendSlice(alloc, v);
                    } else if (envs.get(interpolated_key)) |v| {
                        try value.appendSlice(alloc, v);
                    } else if (args.debug) {
                        try logger.print(
                            tty.yellow ++ "warning:" ++ tty.reset ++ " The " ++ tty.bold ++ tty.yellow ++ "{s}" ++ tty.reset ++ " key contains an undefined or invalid key variable " ++ tty.bold ++ tty.yellow ++ "{s}" ++ tty.reset ++ " ({s}:{d}:{d}); skipping.\n",
                            .{ tkn.key.?, interpolated_key, tkn.file, value_token.line, value_token.byte },
                        );
                    }
                },
                .commented => {
                    if (args.debug) {
                        try logger.print(
                            tty.cyan ++ "info:" ++ tty.reset ++ " Skipping comment {s} ({s}:{d}:{d})\n",
                            .{ value_token.value, tkn.file, value_token.line, value_token.byte },
                        );
                    }
                },
                else => try value.appendSlice(alloc, value_token.value),
            }
        }

        if (tkn.key.?.len > 0 and value.items.len > 0) {
            try envs.put(alloc, try alloc.dupe(u8, tkn.key.?), try alloc.dupe(u8, value.items));
        } else {
            const key_str = if (tkn.key.?.len > 0) tkn.key.? else "(undefined)";
            const val_str = if (value.items.len > 0) value.items else "(undefined)";

            try logger.print(
                tty.yellow ++ "warning:" ++ tty.reset ++ " The " ++ tty.bold ++ tty.yellow ++ "{s}" ++ tty.reset ++ " key contains a " ++ tty.bold ++ tty.yellow ++ "{s}" ++ tty.reset ++ " value ({s}); skipping.\n",
                .{ key_str, val_str, tkn.file },
            );
        }
    }

    const env_count = envs.count();

    if (env_count == 0) {
        try logger.writeAll(
            tty.red ++ "error:" ++ tty.reset ++ " Unable to parse any ENVs! Please ensure the provided .env files are not empty.\n",
        );
        return error.NoParsedEnvs;
    }

    if (args.debug) {
        try logger.print("\n" ++ tty.cyan ++ "info:" ++ tty.reset ++ " Parsed {d} env(s)...\n", .{env_count});
        var i: usize = 0;

        var it = envs.iterator();
        while (it.next()) |entry| {
            i += 1;
            try logger.print("  {d}.) {s}={s}\n", .{ i, entry.key_ptr.*, entry.value_ptr.* });
        }
        try logger.writeByte('\n');
    }

    if (args.required.items.len > 0) {
        var missing: std.ArrayList([]const u8) = .empty;
        defer missing.deinit(alloc);

        for (args.required.items) |req| {
            if (!envs.contains(req)) try missing.append(alloc, req);
        }

        if (missing.items.len > 0) {
            try logger.writeAll(tty.red ++ "error" ++ tty.reset ++ ": The following ENVs were marked as required, but weren't found after parsing: ");
            for (missing.items, 0..) |key, i| {
                if (i != 0) try logger.writeAll(", ");
                try logger.print(tty.bold ++ tty.red ++ "{s}" ++ tty.reset, .{key});
            }
            try logger.writeAll("\nThey're either missing values or weren't set in the provided .env files.\n");
            return error.MissingRequiredEnvs;
        }
    }

    return envs;
}

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
        self.args = .{ .logger = &self.logger };
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

    fn run(self: *TestParse) !std.StringArrayHashMapUnmanaged([]const u8) {
        return parseTokens(&self.environ, self.alloc(), &self.args, &self.tokens, &self.logger);
    }
};

test "parseTokens sets a normalized key/value" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    try tp.addToken("KEY", .normalized, "value");

    const envs = try tp.run();
    try expectEqual(@as(usize, 1), envs.count());
    try expectEqualStrings("value", envs.get("KEY").?);
}

test "parseTokens concatenates multiple value tokens into one value" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    var tok: token.Token = .{ .key = "MULTI", .file = "testp.env" };
    try tok.values.append(tp.alloc(), .{ .kind = .normalized, .value = "12", .line = 1, .byte = 1 });
    try tok.values.append(tp.alloc(), .{ .kind = .multilined, .value = "34", .line = 2, .byte = 1 });
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

    try tp.addToken("FOO", .normalized, "bar");
    try tp.addToken("BAZ", .interpolated, "FOO");

    const envs = try tp.run();
    try expectEqualStrings("bar", envs.get("BAZ").?);
}

test "parseTokens skips an undefined interpolation but keeps valid keys" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    try tp.addToken("VALID", .normalized, "yes");
    try tp.addToken("UNDEF", .interpolated, "NOPE");

    const envs = try tp.run();
    try expectEqual(@as(usize, 1), envs.count());
    try expectEqualStrings("yes", envs.get("VALID").?);
    try expect(envs.get("UNDEF") == null);
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
    try tp.addToken("REQUIRED", .normalized, "ok");

    const envs = try tp.run();
    try expectEqualStrings("ok", envs.get("REQUIRED").?);
}

test "parseTokens errors when a required env is missing" {
    var tp: TestParse = undefined;
    tp.init();
    defer tp.deinit();

    try tp.args.required.append(tp.alloc(), "REQUIRED");
    try tp.addToken("OTHER", .normalized, "x");

    try expectError(error.MissingRequiredEnvs, tp.run());
}
