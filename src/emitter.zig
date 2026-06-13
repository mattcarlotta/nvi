const std = @import("std");
const builtin = @import("builtin");
const arg = @import("arg.zig");
const fmt = @import("formatter.zig");

const Io = std.Io;

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
