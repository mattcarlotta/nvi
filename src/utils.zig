const std = @import("std");
const Io = std.Io;
const mem = std.mem;

const char = @import("char.zig");
const tty = @import("tty.zig");

pub fn isAlphanumeric(s: []const u8) bool {
    if (s.len == 0) return false;
    for (s) |c| {
        if (!std.ascii.isAlphanumeric(c)) return false;
    }
    return true;
}

pub fn isIdentChar(c: u8) bool {
    return std.ascii.isAlphanumeric(c) or c == char.UNDERLINE;
}

pub fn isKeyChar(c: u8) bool {
    return (c >= char.A and c <= char.Z) or (c >= char.ZERO and c <= char.NINE) or c == char.UNDERLINE;
}

pub fn printHelp(out: *Io.Writer) !void {
    try out.writeAll(
        \\Usage: nvi [flags] -- <command>
        \\       nvi scan [ext] [ext] ...etc
        \\
        \\Flags:
        \\  -f, --files <paths>     parses .env files in order (default: .env)
        \\  -i, --ignored <keys>    ignores ENV keys that a scan may find and add to a required ENV key list
        \\  -r, --required <keys>   marks a list of ENV keys that must be present before the command is emitted
        \\  -F, --format <fmt>      outputs ENV format (options: nul|powershell)
        \\  -d, --dry-run           print parsed flags, tokens, scan results, and the resolved env listing to stderr
        \\  -h, --help              prints this help and exits
        \\  -s, --scan <ext>        recursively scans in the root directory for ENV keys used within *.<ext> files and marks them as required †
        \\  -v, --version           prints the version and exits
        \\
        \\Commands:
        \\  help                    prints this help and exits
        \\  scan <ext>              recursively scans in the root directory for ENV keys used within *.<ext> files and marks them as required †
        \\  version                 prints the version and exits
        \\
        \\ † without a command, scan reports what it finds and exits; with a command, the found ENV keys are added to a required ENV key list
        \\
        \\Supported scan file extensions:
        \\ • C -> c
        \\ • Clojure -> clj, cljs, cljc
        \\ • Crystal -> cr
        \\ • C++ -> cc, cpp, cxx, h, hh, hpp, hxx
        \\ • C# -> cs
        \\ • D -> d
        \\ • Dart -> dart
        \\ • Elixir -> ex, exs
        \\ • Erlang -> erl, hrl
        \\ • Fortran -> f, f90, f95, f03, f08, for
        \\ • F# -> fs, fsi, fsx
        \\ • Go -> go
        \\ • Groovy -> groovy
        \\ • Haskell -> hs, lhs
        \\ • Java -> java, gradle
        \\ • JavaScript/TypeScript -> cjs, cts, js, jsx, mjs, mts, ts, tsx
        \\ • Julia -> jl
        \\ • Kotlin -> kt, kts
        \\ • Lua -> lua
        \\ • Nim -> nim
        \\ • Nushell -> nu
        \\ • Objective-C -> m, mm
        \\ • OCaml -> ml, mli
        \\ • Pascal/Delphi -> ldpr, pas, pp
        \\ • Perl -> pl, pm, t
        \\ • PHP -> php
        \\ • PowerShell -> ps1, psm1, psd1
        \\ • Python -> py, pyi
        \\ • R -> r
        \\ • Ruby -> gemspec, rb, rake
        \\ • Rust -> rs
        \\ • Scala -> sc, scala
        \\ • Swift -> swift
        \\ • Tcl -> tcl
        \\ • V -> v
        \\ • Visual Basic -> vb
        \\ • Zig -> zig
        \\
        \\
    );
}

pub fn parseExt(e: []const u8) ![]const u8 {
    var ext = e;
    if (mem.startsWith(u8, ext, "*.")) ext = ext[2..];
    if (mem.startsWith(u8, ext, ".")) ext = ext[1..];

    if (!isAlphanumeric(ext)) {
        return error.InvalidExtension;
    }

    return ext;
}

pub fn sameLineOnly(contents: []const u8, from: usize, to: usize) bool {
    return mem.indexOfScalarPos(u8, contents, from, char.LINE_DELIMITER) == null or
        mem.indexOfScalarPos(u8, contents, from, char.LINE_DELIMITER).? >= to;
}
