const std = @import("std");
const nvi = @import("nvi");

const Io = std.Io;

const Result = enum(u8) { ok = 0, operation_failure = 1, usage_error = 2, info = 3 };

pub fn main(init: std.process.Init) u8 {
    const arena = init.arena.allocator();

    var log_buf: [4096]u8 = undefined;
    var log_writer: Io.File.Writer = Io.File.stderr().writer(init.io, &log_buf);
    const logger = &log_writer.interface;
    defer logger.flush() catch {};

    const argv = init.minimal.args.toSlice(arena) catch {
        logger.writeAll(nvi.tty.red ++ "error" ++ nvi.tty.reset ++ ": Failed to read arguments (out of memory)") catch {};
        return @intFromEnum(Result.operation_failure);
    };

    var args = nvi.args(arena, argv, logger) catch |err| {
        switch (err) {
            error.Help, error.Version => return @intFromEnum(Result.info),
            else => {
                return @intFromEnum(Result.usage_error);
            },
        }
    };

    if (args.scan.items.len > 0) {
        nvi.scanner(init.io, arena, &args, logger) catch {
            return @intFromEnum(Result.operation_failure);
        };

        if (args.command.len == 0) return @intFromEnum(Result.info);
    }

    const tokens = nvi.tokenizer(init.io, arena, &args, logger) catch {
        return @intFromEnum(Result.usage_error);
    };

    const envs = nvi.parser(init.environ_map, arena, &args, &tokens, logger) catch {
        return @intFromEnum(Result.operation_failure);
    };

    var out_buf: [4096]u8 = undefined;
    var out_writer: Io.File.Writer = Io.File.stdout().writer(init.io, &out_buf);
    nvi.emitter(&out_writer.interface, &args, &envs) catch {
        return @intFromEnum(Result.operation_failure);
    };

    return @intFromEnum(Result.ok);
}
