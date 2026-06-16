const std = @import("std");
const utils = @import("utils.zig");

const Io = std.Io;
const mem = std.mem;
const expect = std.testing.expect;
const expectEqualStrings = std.testing.expectEqualStrings;
const expectError = std.testing.expectError;

test "isAlphanumeric accepts letters and digits" {
    try expect(utils.isAlphanumeric("mjs"));
    try expect(utils.isAlphanumeric("ts2"));
    try expect(utils.isAlphanumeric("ZIG"));
}

test "isAlphanumeric rejects empty, punctuation, and whitespace" {
    try expect(!utils.isAlphanumeric(""));
    try expect(!utils.isAlphanumeric("index.mjs"));
    try expect(!utils.isAlphanumeric("src/main"));
    try expect(!utils.isAlphanumeric("a b"));
    try expect(!utils.isAlphanumeric("c_d"));
}

test "isIdentChar accepts alphanumerics and underscore" {
    try expect(utils.isIdentChar('a'));
    try expect(utils.isIdentChar('Z'));
    try expect(utils.isIdentChar('7'));
    try expect(utils.isIdentChar('_'));
}

test "isIdentChar rejects separators and punctuation" {
    try expect(!utils.isIdentChar(' '));
    try expect(!utils.isIdentChar('-'));
    try expect(!utils.isIdentChar('.'));
    try expect(!utils.isIdentChar('"'));
    try expect(!utils.isIdentChar('$'));
}

test "isKeyChar accepts uppercase, digits, and underscore only" {
    try expect(utils.isKeyChar('A'));
    try expect(utils.isKeyChar('Z'));
    try expect(utils.isKeyChar('0'));
    try expect(utils.isKeyChar('9'));
    try expect(utils.isKeyChar('_'));
}

test "isKeyChar rejects lowercase (the boundary that splits keys from identifiers)" {
    try expect(!utils.isKeyChar('a'));
    try expect(!utils.isKeyChar('z'));
    try expect(!utils.isKeyChar('-'));
    try expect(!utils.isKeyChar('.'));
}

test "printHelp writes the usage, flags, and commands" {
    var buf: [4096]u8 = undefined;
    var out: Io.Writer = .fixed(&buf);

    try utils.printHelp(&out);

    const help = out.buffered();
    try expect(mem.indexOf(u8, help, "Usage: nvi [flags] -- <command>") != null);
    try expect(mem.indexOf(u8, help, "-f, --files") != null);
    try expect(mem.indexOf(u8, help, "-i, --ignored") != null);
    try expect(mem.indexOf(u8, help, "-r, --required") != null);
    try expect(mem.indexOf(u8, help, "-F, --format") != null);
    try expect(mem.indexOf(u8, help, "-d, --dry-run") != null);
    try expect(mem.indexOf(u8, help, "-h, --help") != null);
    try expect(mem.indexOf(u8, help, "-v, --version") != null);
    try expect(mem.indexOf(u8, help, "help") != null);
    try expect(mem.indexOf(u8, help, "scan <ext>") != null);
    try expect(mem.indexOf(u8, help, "version") != null);
}

test "parseExt accepts bare, dotted, and glob spellings" {
    try expectEqualStrings("mjs", try utils.parseExt("mjs"));
    try expectEqualStrings("ts", try utils.parseExt(".ts"));
    try expectEqualStrings("zig", try utils.parseExt("*.zig"));
    try expectEqualStrings("Makefile", try utils.parseExt("Makefile"));
}

test "parseExt errors on empty and prefix-only inputs" {
    try expectError(error.InvalidExtension, utils.parseExt(""));
    try expectError(error.InvalidExtension, utils.parseExt("."));
    try expectError(error.InvalidExtension, utils.parseExt("*."));
    try expectError(error.InvalidExtension, utils.parseExt("src/"));
}
