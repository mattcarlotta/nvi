const std = @import("std");
const ac = @import("accessors.zig");
const sc = @import("scanner.zig");

const expect = std.testing.expect;
const expectEqual = std.testing.expectEqual;
const expectEqualStrings = std.testing.expectEqualStrings;

fn expectKeys(ext: []const u8, src: []const u8, expected: []const []const u8) !void {
    var arena = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    const table = ac.extensions.get(ext) orelse {
        try expect(expected.len == 0);
        return;
    };

    var scanner: sc.Scanner = .{
        .io = undefined,
        .alloc = alloc,
        .args = undefined,
        .logger = undefined,
    };
    var matches: std.ArrayList(sc.Scanner.Match) = .empty;
    try scanner.scanContents(src, table, &matches);

    try expectEqual(expected.len, matches.items.len);
    for (expected, matches.items) |want, got| {
        try expectEqualStrings(want, got.key);
    }
}

test "unmapped extensions.type nothing" {
    try expectKeys("xyz", "process.env.NOPE os.getenv(\"NOPE\")", &[_][]const u8{});
    try expectKeys("txt", "ENV[\"NOPE\"]", &[_][]const u8{});
    try expectKeys("sh", "$FOO ${BAR}", &[_][]const u8{});
}

test "JavaScript / TypeScript: ident, bracket, and Vite forms" {
    try expectKeys("ts",
        \\const a = process.env.API_KEY;
        \\const b = process.env["DB_URL"];
        \\const c = import.meta.env.VITE_MODE;
    , &[_][]const u8{ "API_KEY", "DB_URL", "VITE_MODE" });
}

test "JavaScript: dynamic and aliased reads are not.typeed" {
    try expectKeys("ts",
        \\const x = process.env[dynamic];
        \\const e = process.env;
        \\const y = process.env.OK;
    , &[_][]const u8{"OK"});
}

test "shared tables behave identically across their extensions" {
    // JS/TS variants resolve to one table -> same result.
    try expectKeys("ts", "process.env.SAME;", &[_][]const u8{"SAME"});
    try expectKeys("js", "process.env.SAME;", &[_][]const u8{"SAME"});
    try expectKeys("mjs", "process.env.SAME;", &[_][]const u8{"SAME"});

    // Kotlin and Groovy reuse the Java table.
    try expectKeys("java", "System.getenv(\"A\")", &[_][]const u8{"A"});
    try expectKeys("kt", "System.getenv(\"A\")", &[_][]const u8{"A"});
    try expectKeys("groovy", "System.getenv(\"A\")", &[_][]const u8{"A"});
}

test "Python: getenv and environ forms" {
    try expectKeys("py",
        \\x = os.getenv("A")
        \\y = os.environ["B"]
        \\z = os.environ.get("C")
    , &[_][]const u8{ "A", "B", "C" });
}

test "Go: Getenv and LookupEnv" {
    try expectKeys("go",
        \\a := os.Getenv("A")
        \\b, ok := os.LookupEnv("B")
    , &[_][]const u8{ "A", "B" });
}

test "Rust: var and compile-time macros" {
    try expectKeys("rs",
        \\let a = env::var("A").unwrap();
        \\let b = env!("B");
        \\let c = option_env!("C");
    , &[_][]const u8{ "A", "B", "C" });
}

test "Ruby: ENV index and fetch" {
    try expectKeys("rb",
        \\a = ENV["A"]
        \\b = ENV.fetch("B")
    , &[_][]const u8{ "A", "B" });
}

test "C and C++: getenv resolves under both, incl. ambiguous .h" {
    try expectKeys("c", "char *a = getenv(\"A\");", &[_][]const u8{"A"});
    try expectKeys("h", "auto a = std::getenv(\"A\");", &[_][]const u8{"A"});
}

test "PHP: getenv and superglobals" {
    try expectKeys("php",
        \\$a = getenv("A");
        \\$b = $_ENV["B"];
    , &[_][]const u8{ "A", "B" });
}

test "Perl: braced form strips optional quotes" {
    try expectKeys("pl", "my $a = $ENV{A}; my $b = $ENV{'B'};", &[_][]const u8{ "A", "B" });
}

test "PowerShell: sigil ident and .NET accessor" {
    try expectKeys("ps1",
        \\$a = $env:A
        \\$b = [Environment]::GetEnvironmentVariable("B")
    , &[_][]const u8{ "A", "B" });
}

test "Tcl paren form and Nushell dotted form" {
    try expectKeys("tcl", "set a $env(A)", &[_][]const u8{"A"});
    try expectKeys("nu", "let a = $env.A", &[_][]const u8{"A"});
}

test "Elixir: get_env and fetch_env!" {
    try expectKeys("ex",
        \\a = System.get_env("A")
        \\b = System.fetch_env!("B")
    , &[_][]const u8{ "A", "B" });
}

test "Haskell and OCaml: space-call accessors" {
    try expectKeys("hs",
        \\a <- getEnv "A"
        \\b <- lookupEnv "B"
    , &[_][]const u8{ "A", "B" });
    try expectKeys("ml", "let a = Sys.getenv \"A\"", &[_][]const u8{"A"});
}

test "D: index form is caught, `in environment` is not" {
    try expectKeys("d",
        \\auto a = environment["A"];
        \\bool b = ("MISS" in environment);
    , &[_][]const u8{"A"});
}
