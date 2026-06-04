const std = @import("std");
const Io = std.Io;
const print_error = std.log.err;
const exit = std.process.exit;

const nvi = @import("nvi");
const arg = @import("arg.zig");

pub fn main(init: std.process.Init) !void {
    const arena: std.mem.Allocator = init.arena.allocator();
    const argv = init.minimal.args.toSlice(arena) catch {
        print_error("failed to read arguments (out of memory)", .{});
        exit(2);
    };

    var log_buf: [4096]u8 = undefined;
    var log_writer: Io.File.Writer = Io.File.stderr().writer(init.io, &log_buf);
    const logger = &log_writer.interface;

    const args = arg.argParser(arena, argv, logger) catch |err| {
        print_error("argument error: {s}", .{@errorName(err)});
        exit(2);
    };

    if (args.err) |err| {
        try err.print(args.logger);
        try logger.flush();
        exit(2);
    }

    try logger.flush();
}

test {
    _ = @import("arg.zig");
}
