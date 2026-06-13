const std = @import("std");
const tty = @import("tty.zig");
const arg = @import("arg.zig");
const token = @import("tokenizer.zig");
const fmt = @import("formatter.zig");

const Io = std.Io;
const Environ = std.process.Environ;

pub fn parseTokens(
    environ: *Environ.Map,
    alloc: std.mem.Allocator,
    args: *const arg.Arg,
    tokens: *const std.ArrayList(token.Token),
    logger: *Io.Writer,
) !std.StringArrayHashMapUnmanaged([]const u8) {
    var envs: std.StringArrayHashMapUnmanaged([]const u8) = .empty;

    token_loop: for (tokens.items) |tkn| {
        var value: std.ArrayList(u8) = .empty;
        defer value.deinit(alloc);

        for (tkn.values.items) |value_token| {
            switch (value_token.kind) {
                .interpolated => {
                    const interpolated_key = value_token.value;

                    if (environ.get(interpolated_key)) |v| {
                        try value.appendSlice(alloc, v);
                    } else if (envs.get(interpolated_key)) |v| {
                        try value.appendSlice(alloc, v);
                    } else if (args.debug) {
                        try logger.print(
                            tty.yellow ++ "warning:" ++ tty.reset ++ " The " ++ tty.bold_yellow ++ "{s}" ++ tty.reset,
                            .{tkn.key.?},
                        );
                        try logger.print(
                            " key contains an interpolated key variable " ++ tty.bold_yellow ++ "{s}" ++ tty.reset,
                            .{interpolated_key},
                        );
                        try logger.print(
                            " ({s}:{d}:{d}) that is not defined; skipping.\n",
                            .{ tkn.file, value_token.line, value_token.byte },
                        );
                    }
                },
                .commented => {
                    if (args.debug) {
                        try logger.print(
                            tty.cyan ++ "info:" ++ tty.reset ++ " Parsed a comment:" ++ tty.cyan ++ " {s} " ++ tty.reset,
                            .{value_token.value},
                        );
                        try logger.print(
                            "({s}:{d}:{d}); skipping.\n",
                            .{ tkn.file, value_token.line, value_token.byte },
                        );
                    }
                    continue :token_loop;
                },
                else => try value.appendSlice(alloc, value_token.value),
            }
        }

        if (value.items.len == 0) {
            const first = tkn.values.items[0];
            const key_str = if (tkn.key != null) tkn.key.? else "(undefined)";

            try logger.print(
                tty.yellow ++ "warning:" ++ tty.reset ++ " The " ++ tty.bold_yellow ++ "{s}" ++ tty.reset,
                .{key_str},
            );
            try logger.writeAll(" key is an " ++ tty.bold_yellow ++ "undefined" ++ tty.reset);
            try logger.print(
                " value ({s}:{d}:{d}); skipping.\n",
                .{ tkn.file, first.line, first.byte },
            );

            continue :token_loop;
        }

        try envs.put(alloc, tkn.key.?, try value.toOwnedSlice(alloc));
    }

    const env_count = envs.count();

    if (env_count == 0) {
        try logger.writeAll(
            tty.red ++ "error:" ++ tty.reset ++ " Unable to parse any ENVs! Please ensure the provided .env files are not empty.\n",
        );
        return error.NoParsedEnvs;
    }

    if (args.debug) {
        try logger.print("\n" ++ tty.cyan ++ "info:" ++ tty.reset ++ " The following {d} env(s) were parsed and will be emitted...\n", .{env_count});

        var it = envs.iterator();
        while (it.next()) |entry| {
            try logger.writeAll("  • ");
            try fmt.printPair(logger, args.format, entry.key_ptr.*, entry.value_ptr.*);
            try logger.writeByte('\n');
        }
        try logger.writeByte('\n');
    }

    if (args.required.items.len > 0) {
        var missing: std.ArrayList([]const u8) = .empty;
        defer missing.deinit(alloc);

        for (args.required.items) |req| {
            if (!envs.contains(req)) try missing.append(alloc, req);
        }

        if (missing.items.len > 0) {
            try logger.writeAll(tty.red ++ "error" ++ tty.reset ++ ": The following ENV keys were marked as required, but weren't set after parsing: ");

            for (missing.items) |key| {
                try logger.print("\n   • " ++ tty.bold_red ++ "{s}" ++ tty.reset, .{key});
            }

            try logger.writeAll("\nThese keys are either set at runtime by a process or were supposed to be set in an .env file.");
            try logger.writeAll(" You can either ignore them (see help for more info) or include them in an .env file.\n");

            return error.MissingRequiredEnvs;
        }
    }

    return envs;
}
