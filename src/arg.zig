const std = @import("std");
const print_warning = std.log.warn;

pub const Arg = struct {
    files: std.ArrayList([]const u8) = .empty,
    directory: ?[]const u8 = null,
    invalid_flag: ?[]const u8 = null,
    command: ?[]const [:0]const u8 = null,

    const Flag = enum { files, directory };

    const flags = std.StaticStringMap(Flag).initComptime(.{
        .{ "-f", .files },
        .{ "--files", .files },
        .{ "-d", .directory },
        .{ "--directory", .directory },
    });

    pub fn parse(self: *Arg, alloc: std.mem.Allocator, argv: []const [:0]const u8) !void {
        if (argv.len == 0) return;

        var i: usize = 1;
        while (i < argv.len) : (i += 1) {
            const arg = argv[i];

            if (std.mem.eql(u8, arg, "--")) {
                self.command = argv[i + 1 ..];
                break;
            }

            const flag = flags.get(arg) orelse {
                print_warning("ignoring unrecognized argument: {s}", .{arg});
                while (i + 1 < argv.len and flags.get(argv[i + 1]) == null) : (i += 1) {
                    print_warning("ignoring parameter: {s}", .{argv[i + 1]});
                }
                continue;
            };

            if (i + 1 >= argv.len or std.mem.startsWith(u8, argv[i + 1], "-")) {
                self.invalid_flag = argv[i];
                return error.MissingValue;
            }

            i += 1;

            switch (flag) {
                .files => {
                    try self.files.append(alloc, argv[i]);
                    while (i + 1 < argv.len and !std.mem.startsWith(u8, argv[i + 1], "-")) {
                        i += 1;
                        try self.files.append(alloc, argv[i]);
                    }
                },
                .directory => self.directory = argv[i],
            }
        }

        const cmd = self.command orelse return error.MissingCommand;
        if (cmd.len == 0) return error.MissingCommand;
    }
};
