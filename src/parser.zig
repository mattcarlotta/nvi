const std = @import("std");
const tty = @import("tty.zig");
const arg = @import("arg.zig");
const tk = @import("tokenizer.zig");
const fmt = @import("formatter.zig");

const Io = std.Io;
const mem = std.mem;
const Environ = std.process.Environ;

pub const Parser = struct {
    environ: *Environ.Map,
    alloc: mem.Allocator,
    args: *const arg.Arg,
    tokens: *const std.ArrayList(tk.Token),
    logger: *Io.Writer,
    envs: std.StringArrayHashMapUnmanaged([]const u8) = .empty,

    pub fn run(self: *Parser) !void {
        for (self.tokens.items) |tkn| {
            var value: std.ArrayList(u8) = .empty;
            defer value.deinit(self.alloc);

            // a commented token contributes nothing and is skipped whole
            if (try self.resolveValue(tkn, &value)) continue;

            if (value.items.len == 0) {
                try self.warnUndefined(tkn);
                continue;
            }

            try self.envs.put(self.alloc, tkn.key.?, try value.toOwnedSlice(self.alloc));
        }

        if (self.envs.count() == 0) {
            try self.logger.writeAll(
                tty.red ++ "error:" ++ tty.reset ++ " Unable to parse any ENVs! Please ensure the provided .env files are not empty.\n",
            );
            return error.NoParsedEnvs;
        }

        if (self.args.debug) try self.printListing();

        try self.checkRequired();
    }

    fn resolveValue(self: *Parser, tkn: tk.Token, value: *std.ArrayList(u8)) !bool {
        for (tkn.values.items) |value_token| {
            switch (value_token.kind) {
                .interpolated => {
                    const interpolated_key = value_token.value;

                    if (self.environ.get(interpolated_key)) |v| {
                        try value.appendSlice(self.alloc, v);
                    } else if (self.envs.get(interpolated_key)) |v| {
                        try value.appendSlice(self.alloc, v);
                    } else if (self.args.debug) {
                        try self.logger.print(
                            tty.yellow ++ "warning:" ++ tty.reset ++ " The " ++ tty.bold_yellow ++ "{s}" ++ tty.reset,
                            .{tkn.key.?},
                        );
                        try self.logger.print(
                            " key contains an interpolated key variable " ++ tty.bold_yellow ++ "{s}" ++ tty.reset,
                            .{interpolated_key},
                        );
                        try self.logger.print(
                            " ({s}:{d}:{d}) that is not defined; skipping.\n",
                            .{ tkn.file, value_token.line, value_token.byte },
                        );
                    }
                },
                .commented => {
                    if (self.args.debug) {
                        try self.logger.print(
                            tty.cyan ++ "info:" ++ tty.reset ++ " Parsed a comment:" ++ tty.cyan ++ " {s} " ++ tty.reset,
                            .{value_token.value},
                        );
                        try self.logger.print(
                            "({s}:{d}:{d}); skipping.\n",
                            .{ tkn.file, value_token.line, value_token.byte },
                        );
                    }
                    return true;
                },
                else => try value.appendSlice(self.alloc, value_token.value),
            }
        }

        return false;
    }

    fn warnUndefined(self: *Parser, tkn: tk.Token) !void {
        const first = tkn.values.items[0];
        const key_str = if (tkn.key != null) tkn.key.? else "(undefined)";

        try self.logger.print(
            tty.yellow ++ "warning:" ++ tty.reset ++ " The " ++ tty.bold_yellow ++ "{s}" ++ tty.reset,
            .{key_str},
        );
        try self.logger.writeAll(" key is an " ++ tty.bold_yellow ++ "undefined" ++ tty.reset);
        try self.logger.print(
            " value ({s}:{d}:{d}); skipping.\n",
            .{ tkn.file, first.line, first.byte },
        );
    }

    fn printListing(self: *Parser) !void {
        try self.logger.print(
            "\n" ++ tty.cyan ++ "info:" ++ tty.reset ++ " The following {d} env(s) were parsed and will be emitted...\n",
            .{self.envs.count()},
        );

        var it = self.envs.iterator();
        while (it.next()) |entry| {
            try self.logger.writeAll("  • ");
            try fmt.printPair(self.logger, self.args.format, entry.key_ptr.*, entry.value_ptr.*);
            try self.logger.writeByte('\n');
        }

        try self.logger.writeByte('\n');
    }

    fn checkRequired(self: *Parser) !void {
        if (self.args.required.items.len == 0) return;

        var missing: std.ArrayList([]const u8) = .empty;
        defer missing.deinit(self.alloc);

        for (self.args.required.items) |req| {
            if (!self.envs.contains(req)) try missing.append(self.alloc, req);
        }

        if (missing.items.len == 0) return;

        try self.logger.writeAll(tty.red ++ "error" ++ tty.reset ++ ": The following ENV keys were marked as required, but weren't set after parsing: ");

        for (missing.items) |key| {
            try self.logger.print("\n   • " ++ tty.bold_red ++ "{s}" ++ tty.reset, .{key});
        }

        try self.logger.writeAll("\nThese keys are either set at runtime by a process or were supposed to be set in an .env file.");
        try self.logger.writeAll(" You can either ignore them (see help for more info) or include them in an .env file.\n");

        return error.MissingRequiredEnvs;
    }
};
