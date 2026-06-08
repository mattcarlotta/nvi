const std = @import("std");
const Io = std.Io;
const print_error = std.log.err;
const exit = std.process.exit;

const nvi = @import("nvi");

pub fn main(init: std.process.Init) !u8 {
    const arena = init.arena.allocator();

    var buf: [4096]u8 = undefined;
    var writer: Io.File.Writer = Io.File.stderr().writer(init.io, &buf);
    const logger = &writer.interface;
    defer logger.flush() catch {};

    const argv = init.minimal.args.toSlice(arena) catch {
        try logger.writeAll("failed to read arguments (out of memory)");
        return 2;
    };

    const args = nvi.args(arena, argv, logger) catch {
        return 2;
    };

    _ = nvi.tokenizer(init.io, arena, &args, logger) catch {
        return 2;
    };

    // const envs: std.StringArrayHashMapUnmanaged([]const u8) = .empty;

    return 0;
}
