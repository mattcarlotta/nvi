const std = @import("std");
const builtin = @import("builtin");

const Io = std.Io;
const mem = std.mem;
const expectEqualStrings = std.testing.expectEqualStrings;

pub const Format = enum {
    nul,
    powershell,

    pub const default: Format = if (builtin.os.tag == .windows) .powershell else .nul;
};

pub fn printPair(out: *Io.Writer, format: Format, key: []const u8, value: []const u8) !void {
    switch (format) {
        .nul => try out.print("{s}={s}", .{ key, value }),
        .powershell => {
            try out.print("$env:{s} = '", .{key});
            try writePsQuoted(out, value);
            try out.writeByte('\'');
        },
    }
}

pub fn writePsQuoted(out: *Io.Writer, s: []const u8) !void {
    var start: usize = 0;
    while (mem.indexOfScalarPos(u8, s, start, '\'')) |i| {
        try out.writeAll(s[start .. i + 1]);
        try out.writeByte('\'');
        start = i + 1;
    }
    try out.writeAll(s[start..]);
}

const TestFormat = struct {
    buf: [256]u8 = undefined,
    out: Io.Writer = undefined,

    fn pair(self: *TestFormat, format: Format, key: []const u8, value: []const u8) ![]const u8 {
        self.out = .fixed(&self.buf);
        try printPair(&self.out, format, key, value);
        return self.out.buffered();
    }
};

test "nul pair is key=value" {
    var t: TestFormat = .{};
    try expectEqualStrings("A=two words", try t.pair(.nul, "A", "two words"));
}

test "powershell pair is a single-quoted env assignment" {
    var t: TestFormat = .{};
    try expectEqualStrings("$env:A = 'two words'", try t.pair(.powershell, "A", "two words"));
}

test "powershell pair escapes single quotes by doubling" {
    var t: TestFormat = .{};
    try expectEqualStrings("$env:MSG = 'it''s o''clock'", try t.pair(.powershell, "MSG", "it's o'clock"));
}

test "powershell pair preserves newlines inside single quotes" {
    var t: TestFormat = .{};
    try expectEqualStrings("$env:MULTI = 'line1\nline2'", try t.pair(.powershell, "MULTI", "line1\nline2"));
}
