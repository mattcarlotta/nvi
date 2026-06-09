const std = @import("std");
const tty = @import("tty.zig");
const arg = @import("arg.zig");
const token = @import("tokenizer.zig");
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

    for (tokens.items) |tk| {
        if (tk.values.items.len == 0 or tk.key == null) continue;

        var value: std.ArrayList(u8) = .empty;
        defer value.deinit(alloc);

        for (tk.values.items) |value_token| {
            if (value_token.value.len == 0) continue;

            switch (value_token.kind) {
                .interpolated => {
                    const interpolated_key = value_token.value;

                    if (environ.get(interpolated_key)) |v| {
                        try value.appendSlice(alloc, v);
                    } else if (envs.get(interpolated_key)) |v| {
                        try value.appendSlice(alloc, v);
                    } else if (args.debug) {
                        try logger.print(
                            tty.yellow ++ "warning:" ++ tty.reset ++ " The " ++ tty.bold ++ tty.yellow ++ "{s}" ++ tty.reset ++ " key references an undefined variable " ++ tty.bold ++ tty.yellow ++ "{s}" ++ tty.reset ++ " ({s}:{d}:{d}); skipping.\n",
                            .{ tk.key.?, interpolated_key, tk.file, value_token.line, value_token.byte },
                        );
                    }
                },
                .commented => {
                    if (args.debug) {
                        try logger.print(
                            tty.cyan ++ "info:" ++ tty.reset ++ " Skipping comment {s} ({s}:{d}:{d})\n",
                            .{ value_token.value, tk.file, value_token.line, value_token.byte },
                        );
                    }
                },
                else => try value.appendSlice(alloc, value_token.value),
            }
        }

        if (tk.key.?.len > 0 and value.items.len > 0) {
            try envs.put(alloc, try alloc.dupe(u8, tk.key.?), try alloc.dupe(u8, value.items));
        } else {
            const key_str = if (tk.key.?.len > 0) tk.key.? else "(undefined)";
            const val_str = if (value.items.len > 0) value.items else "(undefined)";

            try logger.print(
                tty.yellow ++ "warning:" ++ tty.reset ++ " The " ++ tty.bold ++ tty.yellow ++ "{s}" ++ tty.reset ++ " key with a(n) " ++ tty.bold ++ tty.yellow ++ "{s}" ++ tty.reset ++ " value was not set ({s}).\n",
                .{ key_str, val_str, tk.file },
            );
        }
    }

    const env_count = envs.count();

    if (env_count == 0) {
        try logger.writeAll(
            tty.red ++ "error:" ++ tty.reset ++ " Unable to parse any ENVs! Please ensure the provided .env files are not empty.\n",
        );
        return error.NoParsedEnvs;
    }

    if (args.debug) {
        try logger.print("\n" ++ tty.cyan ++ "info:" ++ tty.reset ++ " Parsed {d} env(s)...\n", .{env_count});
        var i: usize = 0;

        var it = envs.iterator();
        while (it.next()) |entry| {
            i += 1;
            try logger.print("  {d}.) {s}={s}\n", .{ i, entry.key_ptr.*, entry.value_ptr.* });
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
            try logger.writeAll(tty.red ++ "error" ++ tty.reset ++ ": The following ENVs were marked as required, but weren't found after parsing: ");
            for (missing.items, 0..) |key, i| {
                if (i != 0) try logger.writeAll(", ");
                try logger.print(tty.bold ++ tty.red ++ "{s}" ++ tty.reset, .{key});
            }
            try logger.writeAll("\nThey're either missing values or weren't set in the provided .env files.\n");
            return error.MissingRequiredEnvs;
        }
    }

    return envs;
}
