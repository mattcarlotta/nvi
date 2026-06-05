const std = @import("std");
const Io = std.Io;
const print_error = std.log.err;
const exit = std.process.exit;

const nvi = @import("nvi");

pub fn main(init: std.process.Init) !u8 {
    var log_buf: [4096]u8 = undefined;
    var log_writer: Io.File.Writer = Io.File.stderr().writer(init.io, &log_buf);
    const logger = &log_writer.interface;
    defer logger.flush() catch {};

    const arena = init.arena.allocator();
    const argv = init.minimal.args.toSlice(arena) catch {
        try logger.writeAll("failed to read arguments (out of memory)");
        return 2;
    };

    const args = nvi.argParser(arena, argv, logger) catch |err| {
        try logger.print("argument error: {s}", .{@errorName(err)});
        return 2;
    };

    if (args.err) |err| {
        const msg = try err.getMessage(arena);
        try logger.writeAll(msg);
        return 2;
    }

    return 0;
}
