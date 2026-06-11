const std = @import("std");
const builtin = @import("builtin");
const arg = @import("arg.zig");
const fmt = @import("formatter.zig");

const Io = std.Io;
const mem = std.mem;
const expectEqualStrings = std.testing.expectEqualStrings;

pub fn emitter(
    out: *Io.Writer,
    args: *const arg.Arg,
    envs: *const std.StringArrayHashMapUnmanaged([]const u8),
) !void {
    switch (args.format) {
        .nul => try emitNul(out, envs, args.command),
        .powershell => try emitPowershell(out, envs, args.command),
    }

    try out.flush();
}

fn emitNul(
    out: *Io.Writer,
    envs: *const std.StringArrayHashMapUnmanaged([]const u8),
    command: []const [:0]const u8,
) !void {
    var it = envs.iterator();
    while (it.next()) |entry| {
        try fmt.printPair(out, .nul, entry.key_ptr.*, entry.value_ptr.*);
        try out.writeByte(0);
    }

    for (command) |part| {
        try out.writeAll(part);
        try out.writeByte(0);
    }
}

fn emitPowershell(
    out: *Io.Writer,
    envs: *const std.StringArrayHashMapUnmanaged([]const u8),
    command: []const [:0]const u8,
) !void {
    var it = envs.iterator();
    while (it.next()) |entry| {
        try fmt.printPair(out, .powershell, entry.key_ptr.*, entry.value_ptr.*);
        try out.writeByte('\n');
    }

    if (command.len > 0) {
        try out.writeAll("&");
        for (command) |part| {
            try out.writeAll(" '");
            try fmt.writePsQuoted(out, part);
            try out.writeByte('\'');
        }
        try out.writeByte('\n');
    }
}

const TestEmit = struct {
    arena: std.heap.ArenaAllocator,
    buf: [256]u8 = undefined,
    logger_buf: [256]u8 = undefined,
    out: Io.Writer = undefined,
    logger: Io.Writer = undefined,
    envs: std.StringArrayHashMapUnmanaged([]const u8) = .empty,

    fn init() TestEmit {
        return .{ .arena = std.heap.ArenaAllocator.init(std.testing.allocator) };
    }

    fn deinit(self: *TestEmit) void {
        self.arena.deinit();
    }

    fn put(self: *TestEmit, key: []const u8, value: []const u8) !void {
        try self.envs.put(self.arena.allocator(), key, value);
    }

    fn run(self: *TestEmit, format: fmt.Format, command: []const [:0]const u8) ![]const u8 {
        self.out = .fixed(&self.buf);
        self.logger = .fixed(&self.logger_buf);

        const args: arg.Arg = .{
            .logger = &self.logger,
            .format = format,
            .command = command,
        };

        try emitter(&self.out, &args, &self.envs);
        return self.out.buffered();
    }
};

test "nul: emits NUL-delimited pairs then command tokens" {
    var t = TestEmit.init();
    defer t.deinit();

    try t.put("A", "1");
    try t.put("B", "two words");

    try expectEqualStrings("A=1\x00B=two words\x00echo\x00hi\x00", try t.run(.nul, &.{ "echo", "hi" }));
}

test "nul: emits values containing spaces and newlines byte-exact" {
    var t = TestEmit.init();
    defer t.deinit();

    try t.put("MULTI", "line1\nline2");

    try expectEqualStrings("MULTI=line1\nline2\x00env\x00", try t.run(.nul, &.{"env"}));
}

test "nul: emits pairs only when the command is empty" {
    var t = TestEmit.init();
    defer t.deinit();

    try t.put("A", "1");

    try expectEqualStrings("A=1\x00", try t.run(.nul, &.{}));
}

test "powershell: emits assignments then a call-operator invocation" {
    var t = TestEmit.init();
    defer t.deinit();

    try t.put("A", "1");
    try t.put("B", "two words");

    try expectEqualStrings(
        "$env:A = '1'\n$env:B = 'two words'\n& 'echo' 'hi'\n",
        try t.run(.powershell, &.{ "echo", "hi" }),
    );
}

test "powershell: escapes single quotes by doubling" {
    var t = TestEmit.init();
    defer t.deinit();

    try t.put("MSG", "it's o'clock");

    try expectEqualStrings(
        "$env:MSG = 'it''s o''clock'\n& 'env'\n",
        try t.run(.powershell, &.{"env"}),
    );
}

test "powershell: preserves newlines inside single quotes" {
    var t = TestEmit.init();
    defer t.deinit();

    try t.put("MULTI", "line1\nline2");

    try expectEqualStrings(
        "$env:MULTI = 'line1\nline2'\n& 'env'\n",
        try t.run(.powershell, &.{"env"}),
    );
}

test "powershell: emits assignments only when the command is empty" {
    var t = TestEmit.init();
    defer t.deinit();

    try t.put("A", "1");

    try expectEqualStrings("$env:A = '1'\n", try t.run(.powershell, &.{}));
}
