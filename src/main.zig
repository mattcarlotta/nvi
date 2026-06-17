const std = @import("std");
const Io = std.Io;

const nvi = @import("nvi");

// Entry point and pipeline orchestrator. Sets up the stderr writer, parses
// args, then runs scan (optional) -> tokenize -> parse -> emit, mapping each
// stage's outcome to a process exit code (ok/operation_failure/usage_error).
//
// Diagnostics buffer on stderr and are flushed before any data is written to
// stdout, so the two streams never interleave; stdout is reserved for the emit.

const Result = enum(u8) { ok = 0, operation_failure = 1, usage_error = 2 };

pub fn main(init: std.process.Init) u8 {
    const alloc = init.arena.allocator();

    var stderr_buf: [4096]u8 = undefined;
    var stderr_writer: Io.File.Writer = Io.File.stderr().writer(init.io, &stderr_buf);
    const stderr = &stderr_writer.interface;
    defer stderr.flush() catch {};

    const argv = init.minimal.args.toSlice(alloc) catch {
        stderr.writeAll(nvi.tty.red ++ "error" ++ nvi.tty.reset ++ ": Failed to read arguments (out of memory)") catch {};
        return @intFromEnum(Result.operation_failure);
    };

    var args: nvi.Arg = .{ .alloc = alloc, .argv = argv, .logger = stderr };

    args.run() catch |err| {
        switch (err) {
            error.Help, error.Version => return @intFromEnum(Result.ok),
            else => {
                return @intFromEnum(Result.usage_error);
            },
        }
    };

    if (args.scan.items.len > 0) {
        var scanner: nvi.Scanner = .{ .alloc = alloc, .args = &args, .io = init.io, .logger = stderr };

        scanner.run() catch |err| {
            switch (err) {
                error.NoCommand => return @intFromEnum(Result.ok),
                else => return @intFromEnum(Result.operation_failure),
            }
        };
    }

    var tokenizer: nvi.Tokenizer = .{ .alloc = alloc, .args = &args, .io = init.io, .logger = stderr };

    tokenizer.run() catch {
        return @intFromEnum(Result.operation_failure);
    };

    var parser: nvi.Parser = .{ .alloc = alloc, .args = &args, .environ = init.environ_map, .tokens = &tokenizer.tokens, .logger = stderr };

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

    stderr.flush() catch {};

    var stdout_buf: [4096]u8 = undefined;
    var stdout_writer: Io.File.Writer = Io.File.stdout().writer(init.io, &stdout_buf);
    const stdout = &stdout_writer.interface;
    defer stdout.flush() catch {};

    nvi.emitter(stdout, &args, &parser.envs) catch {
        return @intFromEnum(Result.operation_failure);
    };

    stdout.flush() catch {};

    return @intFromEnum(Result.ok);
}
