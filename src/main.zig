const std = @import("std");
const Io = std.Io;

const nvi = @import("nvi");

const Result = enum(u8) { ok = 0, operation_failure = 1, usage_error = 2 };

pub fn main(init: std.process.Init) u8 {
    const io = init.io;
    const alloc = init.arena.allocator();

    const stderr: Io.File = .stderr();
    var stderr_buf: [4096]u8 = undefined;
    var stderr_writer = stderr.writer(io, &stderr_buf);
    const werr = &stderr_writer.interface;
    defer werr.flush() catch {};

    const argv = init.minimal.args.toSlice(alloc) catch {
        werr.writeAll(nvi.tty.red ++ "error" ++ nvi.tty.reset ++ ": Failed to read arguments (out of memory)") catch {};
        return @intFromEnum(Result.operation_failure);
    };

    var args: nvi.Arg = .{ .alloc = alloc, .argv = argv, .logger = werr };

    args.run() catch |err| {
        switch (err) {
            error.Help, error.Version => return @intFromEnum(Result.ok),
            else => {
                return @intFromEnum(Result.usage_error);
            },
        }
    };

    if (args.scan.items.len > 0) {
        var scanner: nvi.Scanner = .{ .alloc = alloc, .args = &args, .io = io, .logger = werr };

        scanner.run() catch |err| {
            switch (err) {
                error.NoCommand => return @intFromEnum(Result.ok),
                else => return @intFromEnum(Result.operation_failure),
            }
        };
    }

    var tokenizer: nvi.Tokenizer = .{ .alloc = alloc, .args = &args, .io = io, .logger = werr };

    tokenizer.run() catch {
        return @intFromEnum(Result.operation_failure);
    };

    var parser: nvi.Parser = .{ .alloc = alloc, .args = &args, .environ = init.environ_map, .tokens = &tokenizer.tokens, .logger = werr };

    parser.run() catch {
        return @intFromEnum(Result.operation_failure);
    };

    if (args.dry_run) {
        return @intFromEnum(Result.ok);
    }

    if (args.command.len == 0) {
        args.printMissingCommand();
        return @intFromEnum(Result.usage_error);
    }

    werr.flush() catch {
        return @intFromEnum(Result.operation_failure);
    };

    const stdout: Io.File = .stdout();
    var stdout_buf: [4096]u8 = undefined;
    var stdout_writer = stdout.writer(io, &stdout_buf);
    const wout = &stdout_writer.interface;
    defer wout.flush() catch {};

    nvi.emitter(wout, &args, &parser.envs) catch {
        return @intFromEnum(Result.operation_failure);
    };

    wout.flush() catch {
        return @intFromEnum(Result.operation_failure);
    };

    return @intFromEnum(Result.ok);
}
