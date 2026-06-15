const std = @import("std");
const nvi = @import("nvi");

const Io = std.Io;

const Result = enum(u8) { ok = 0, operation_failure = 1, usage_error = 2 };

pub fn main(init: std.process.Init) !u8 {
    const arena = init.arena.allocator();

    var log_buf: [4096]u8 = undefined;
    var log_writer: Io.File.Writer = Io.File.stderr().writer(init.io, &log_buf);
    const logger = &log_writer.interface;
    defer logger.flush() catch {};

    const argv = init.minimal.args.toSlice(arena) catch {
        try logger.writeAll(nvi.tty.red ++ "error" ++ nvi.tty.reset ++ ": Failed to read arguments (out of memory)");
        return @intFromEnum(Result.operation_failure);
    };

    var args = nvi.args(arena, argv, logger) catch |err| {
        switch (err) {
            error.Help, error.Version => return @intFromEnum(Result.ok),
            else => {
                return @intFromEnum(Result.usage_error);
            },
        }
    };

    const no_command = args.command.len == 0;

    if (args.scan.items.len > 0) {
        nvi.scanner(init.io, arena, &args, logger) catch {
            return @intFromEnum(Result.operation_failure);
        };

        if (no_command) return @intFromEnum(Result.ok);
    }

    const tokens = nvi.tokenizer(init.io, arena, &args, logger) catch {
        return @intFromEnum(Result.usage_error);
    };

    const envs = nvi.parser(init.environ_map, arena, &args, &tokens, logger) catch {
        return @intFromEnum(Result.operation_failure);
    };

    logger.flush() catch {};

    if (no_command) {
        try args.logger.writeAll(nvi.tty.red ++ "error:" ++ nvi.tty.reset ++ " An " ++ nvi.tty.italic ++ "end of options delimiter" ++ nvi.tty.reset);
        try args.logger.writeAll(" (--) must be defined and followed by a command (e.g., nvi <flags>" ++ nvi.tty.green ++ " -- <command>" ++ nvi.tty.reset ++ "). See nvi help.\n");
        return @intFromEnum(Result.usage_error);
    }

    var stdout_buf: [4096]u8 = undefined;
    var stdout_writer: Io.File.Writer = Io.File.stdout().writer(init.io, &stdout_buf);
    nvi.emitter(&stdout_writer.interface, &args, &envs) catch {
        return @intFromEnum(Result.operation_failure);
    };

    return @intFromEnum(Result.ok);
}
