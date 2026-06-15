const std = @import("std");
const ac = @import("accessors.zig");

const mem = std.mem;
const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;

fn shapeOf(table: []const ac.Accessor, prefix: []const u8) ?ac.Extract {
    for (table) |a| {
        if (mem.eql(u8, a.prefix, prefix)) return a.extract;
    }
    return null;
}

test "forExt returns null for unknown or non-source extensions" {
    try expect(ac.forExt("xyz") == null);
    try expect(ac.forExt("txt") == null);
    try expect(ac.forExt("md") == null);
    try expect(ac.forExt("json") == null);
    try expect(ac.forExt("") == null);
}

test "forExt resolves the common languages" {
    const known = [_][]const u8{
        "ts",  "js",   "py",  "go",  "rs",   "rb",
        "zig", "java", "kt",  "cs",  "php",  "swift",
        "lua", "rs",   "ex",  "erl", "hs",   "ml",
        "c",   "cpp",  "ps1", "pl",  "tcl",  "nim",
        "d",   "cr",   "jl",  "r",   "dart",
    };
    for (known) |ext| {
        try expect(ac.forExt(ext) != null);
    }
}

test "shell extensions are opt-in and not mapped by default" {
    try expect(ac.forExt("sh") == null);
    try expect(ac.forExt("bash") == null);
    try expect(ac.forExt("zsh") == null);
    try expect(ac.forExt("ksh") == null);
}

test "extensions that share a language share one table" {
    const ts = ac.forExt("ts").?;
    try expectEqual(ts.ptr, ac.forExt("js").?.ptr);
    try expectEqual(ts.ptr, ac.forExt("mjs").?.ptr);
    try expectEqual(ts.ptr, ac.forExt("tsx").?.ptr);
    try expectEqual(ts.ptr, ac.forExt("cts").?.ptr);

    try expectEqual(ac.forExt("java").?.ptr, ac.forExt("kt").?.ptr);
    try expectEqual(ac.forExt("java").?.ptr, ac.forExt("groovy").?.ptr);

    const cpp = ac.forExt("cpp").?;
    try expectEqual(cpp.ptr, ac.forExt("cc").?.ptr);
    try expectEqual(cpp.ptr, ac.forExt("hpp").?.ptr);
    try expectEqual(cpp.ptr, ac.forExt("h").?.ptr);

    try expectEqual(ac.forExt("cs").?.ptr, ac.forExt("fs").?.ptr);

    try expectEqual(ac.forExt("f90").?.ptr, ac.forExt("f").?.ptr);
}

test "C and C++ are distinct tables" {
    try expect(ac.forExt("c").?.ptr != ac.forExt("cpp").?.ptr);
}

test "JavaScript table carries the expected accessors and shapes" {
    const t = ac.forExt("ts").?;
    try expectEqual(ac.Extract.ident, shapeOf(t, "process.env.").?);
    try expectEqual(ac.Extract.quoted, shapeOf(t, "process.env[").?);
    try expectEqual(ac.Extract.ident, shapeOf(t, "import.meta.env.").?);
}

test "Python table carries the expected accessors" {
    const t = ac.forExt("py").?;
    try expectEqual(ac.Extract.quoted, shapeOf(t, "os.getenv(").?);
    try expectEqual(ac.Extract.quoted, shapeOf(t, "os.environ[").?);
    try expectEqual(ac.Extract.quoted, shapeOf(t, "os.environ.get(").?);
}

test "Go and Ruby tables carry the expected accessors" {
    const g = ac.forExt("go").?;
    try expectEqual(ac.Extract.quoted, shapeOf(g, "os.Getenv(").?);
    try expectEqual(ac.Extract.quoted, shapeOf(g, "os.LookupEnv(").?);

    const r = ac.forExt("rb").?;
    try expectEqual(ac.Extract.quoted, shapeOf(r, "ENV[").?);
    try expectEqual(ac.Extract.quoted, shapeOf(r, "ENV.fetch(").?);
}

test "Rust compile-time macros extract a quoted key" {
    const t = ac.forExt("rs").?;
    try expectEqual(ac.Extract.quoted, shapeOf(t, "env!(").?);
    try expectEqual(ac.Extract.quoted, shapeOf(t, "option_env!(").?);
    try expectEqual(ac.Extract.quoted, shapeOf(t, "std::env::var(").?);
}

test "sigil-based languages use the right extraction shapes" {
    try expectEqual(ac.Extract.ident, shapeOf(ac.forExt("ps1").?, "$env:").?);
    try expectEqual(ac.Extract.braced, shapeOf(ac.forExt("pl").?, "$ENV{").?);
    try expectEqual(ac.Extract.parened, shapeOf(ac.forExt("tcl").?, "$env(").?);
    try expectEqual(ac.Extract.ident, shapeOf(ac.forExt("nu").?, "$env.").?);
}

test "an absent accessor reports null via shapeOf" {
    const t = ac.forExt("go").?;
    try expect(shapeOf(t, "process.env.") == null);
}
