const std = @import("std");
const nvi = @import("nvi");

const Io = std.Io;
const exit = std.process.exit;

pub fn main(init: std.process.Init) !u8 {
    const arena = init.arena.allocator();

    var log_buf: [4096]u8 = undefined;
    var log_writer: Io.File.Writer = Io.File.stderr().writer(init.io, &log_buf);
    const logger = &log_writer.interface;
    defer logger.flush() catch {};

    const argv = init.minimal.args.toSlice(arena) catch {
        try logger.writeAll("failed to read arguments (out of memory)");
        return 2;
    };

    const args = nvi.args(arena, argv, logger) catch {
        return 2;
    };

    const tokens = nvi.tokenizer(init.io, arena, &args, logger) catch {
        return 2;
    };

    const envs = nvi.parser(init.environ_map, arena, &args, &tokens, logger) catch {
        return 2;
    };

    var out_buf: [4096]u8 = undefined;
    var out_writer: Io.File.Writer = Io.File.stdout().writer(init.io, &out_buf);
    try nvi.emitter(&out_writer.interface, &args, &envs);

    return 0;
}
