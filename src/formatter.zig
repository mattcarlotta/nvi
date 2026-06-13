const std = @import("std");
const builtin = @import("builtin");

const Io = std.Io;
const mem = std.mem;

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
