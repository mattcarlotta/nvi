const std = @import("std");
const builtin = @import("builtin");
const fmt = @import("formatter.zig");

const Io = std.Io;
const mem = std.mem;
const expectEqualStrings = std.testing.expectEqualStrings;

const TestFormat = struct {
    buf: [256]u8 = undefined,
    out: Io.Writer = undefined,

    fn pair(self: *TestFormat, format: fmt.Format, key: []const u8, value: []const u8) ![]const u8 {
        self.out = .fixed(&self.buf);
        try fmt.printPair(&self.out, format, key, value);
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
