const std = @import("std");
const tty = @import("tty.zig");

const Io = std.Io;
const mem = std.mem;

pub fn isAlphanumeric(s: []const u8) bool {
    if (s.len == 0) return false;
    for (s) |c| {
        if (!std.ascii.isAlphanumeric(c)) return false;
    }
    return true;
}

pub fn isIdentChar(c: u8) bool {
    return std.ascii.isAlphanumeric(c) or c == '_';
}

pub fn isKeyChar(c: u8) bool {
    return (c >= 'A' and c <= 'Z') or (c >= '0' and c <= '9') or c == '_';
}

pub fn printHelp(out: *Io.Writer) !void {
    try out.writeAll(
        \\Usage: nvi [flags] -- <command>
        \\       nvi scan [ext]
        \\
        \\Flags:
        \\  -f, --files <paths>     parses .env files in order (default: .env)
        \\  -i, --ignored <keys>    ignores ENV keys that a scan may add to the required ENV key list
        \\  -r, --required <keys>   marks a list of ENV keys that must be present
        \\  -F, --format <fmt>      outputs ENV format (options: nul|powershell)
        \\  -d, --debug             prints internal debug details to stderr
        \\
        \\Commands:
        \\  help                    prints this help and exits
        \\  scan <ext>              recursively scans in the root directory for ENV keys used within *.<ext> files and marks them as required †
        \\  version                 prints the version and exits
        \\
        \\ † without a command, scan reports what it finds and exits; with a command, the found ENV keys are added to the required ENV set
        \\
        \\Example:
        \\  nvi --files .env -- cargo run | xargs -0 env
        \\  nvi scan *.ts --ignored NODE_ENV -- npm run dev | xargs -0 env
        \\
    );
}

pub fn parseExt(e: []const u8, logger: *Io.Writer) ![]const u8 {
    var ext = e;
    if (mem.startsWith(u8, ext, "*.")) ext = ext[2..];
    if (mem.startsWith(u8, ext, ".")) ext = ext[1..];

    if (!isAlphanumeric(ext)) {
        try logger.print(tty.red ++ "error:" ++ tty.reset ++ " The scan parameter " ++ tty.bold_red ++ "{s}" ++ tty.reset ++ " is not a file extension (e.g. mjs or *.mjs). If you passed an unquoted glob, your shell may have expanded it into filenames; pass the bare extension instead\n", .{e});
        return error.InvalidExtension;
    }

    return ext;
}
