const std = @import("std");
const Io = std.Io;

const arg = @import("arg.zig");
const fmt = @import("formatter.zig");

pub fn emitter(
    out: *Io.Writer,
    args: *const arg.Arg,
    envs: *const std.StringArrayHashMapUnmanaged([]const u8),
) !void {
    switch (args.format) {
        .nul => {
            var it = envs.iterator();
            while (it.next()) |entry| {
                try fmt.printPair(out, .nul, entry.key_ptr.*, entry.value_ptr.*);
                try out.writeByte(0);
            }

            for (args.command) |part| {
                try out.writeAll(part);
                try out.writeByte(0);
            }
        },
        .powershell => {
            var it = envs.iterator();
            while (it.next()) |entry| {
                try fmt.printPair(out, .powershell, entry.key_ptr.*, entry.value_ptr.*);
                try out.writeByte('\n');
            }

            if (args.command.len > 0) {
                try out.writeAll("&");
                for (args.command) |part| {
                    try out.writeAll(" '");
                    try fmt.writePsQuoted(out, part);
                    try out.writeByte('\'');
                }
                try out.writeByte('\n');
            }
        },
    }
}
