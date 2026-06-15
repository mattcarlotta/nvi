const std = @import("std");

// Env-variable access patterns, grouped by language and keyed by file
// extension. The scanner matches `prefix` at a position, then.patterns the key
// using .pattern`. This replaces convention-based suffix matching (`_ENV`) with
// intent-based matching: a token is an env var because it is read through a
// known environment accessor, not because of how it is spelled.
//
// WHAT THIS CANNOT CATCH (true of every language, lexical limit):
//   - destructuring:   const { DATABASE_URL } = process.env
//   - aliasing:        const e = process.env; e.FOO
//   - dynamic keys:    process.env[name]        os.getenv(var)

pub const Pattern = enum {
    ident, // bareword identifier follows the prefix:        process.env.FOO
    quoted, // first string literal ("..."/'...') follows:    getenv("FOO")
    braced, // token (bareword or quoted) up to '}':          $ENV{FOO}  ${FOO}
    parened, // bareword up to ')':                            $env(FOO)
};

pub const Accessor = struct {
    prefix: []const u8,
    pattern: Pattern,
};

const javascript = [_]Accessor{
    .{ .prefix = "process.env.", .pattern = .ident },
    .{ .prefix = "process.env[", .pattern = .quoted },
    .{ .prefix = "import.meta.env.", .pattern = .ident },
    .{ .prefix = "import.meta.env[", .pattern = .quoted },
};

const deno_bun = [_]Accessor{
    .{ .prefix = "Deno.env.get(", .pattern = .quoted },
    .{ .prefix = "Deno.env.has(", .pattern = .quoted },
    .{ .prefix = "Bun.env.", .pattern = .ident },
    .{ .prefix = "Bun.env[", .pattern = .quoted },
};

const python = [_]Accessor{
    .{ .prefix = "os.getenv(", .pattern = .quoted },
    .{ .prefix = "os.environ[", .pattern = .quoted },
    .{ .prefix = "os.environ.get(", .pattern = .quoted },
    .{ .prefix = "os.environ.setdefault(", .pattern = .quoted },
    .{ .prefix = "getenv(", .pattern = .quoted },
    .{ .prefix = "environ[", .pattern = .quoted },
    .{ .prefix = "environ.get(", .pattern = .quoted },
};

const go = [_]Accessor{
    .{ .prefix = "os.Getenv(", .pattern = .quoted },
    .{ .prefix = "os.LookupEnv(", .pattern = .quoted },
};

const rust = [_]Accessor{
    .{ .prefix = "std::env::var(", .pattern = .quoted },
    .{ .prefix = "std::env::var_os(", .pattern = .quoted },
    .{ .prefix = "env::var(", .pattern = .quoted },
    .{ .prefix = "env::var_os(", .pattern = .quoted },
    .{ .prefix = "env!(", .pattern = .quoted },
    .{ .prefix = "option_env!(", .pattern = .quoted },
};

const ruby = [_]Accessor{
    .{ .prefix = "ENV[", .pattern = .quoted },
    .{ .prefix = "ENV.fetch(", .pattern = .quoted },
};

const zig = [_]Accessor{
    .{ .prefix = "std.posix.getenv(", .pattern = .quoted },
    .{ .prefix = "posix.getenv(", .pattern = .quoted },
    .{ .prefix = "std.process.getEnvVarOwned(", .pattern = .quoted },
    .{ .prefix = "std.process.hasEnvVar(", .pattern = .quoted },
    .{ .prefix = "std.process.hasEnvVarConstant(", .pattern = .quoted },
    .{ .prefix = "std.process.parseEnvVarInt(", .pattern = .quoted },
};

const java = [_]Accessor{
    .{ .prefix = "System.getenv(", .pattern = .quoted },
};

const scala = [_]Accessor{
    .{ .prefix = "sys.env(", .pattern = .quoted },
    .{ .prefix = "sys.env.get(", .pattern = .quoted },
    .{ .prefix = "System.getenv(", .pattern = .quoted },
};

const clojure = [_]Accessor{
    .{ .prefix = "System/getenv ", .pattern = .quoted },
    .{ .prefix = "(getenv ", .pattern = .quoted },
};

const c = [_]Accessor{
    .{ .prefix = "getenv(", .pattern = .quoted },
    .{ .prefix = "secure_getenv(", .pattern = .quoted },
};

const cpp = [_]Accessor{
    .{ .prefix = "std::getenv(", .pattern = .quoted },
    .{ .prefix = "getenv(", .pattern = .quoted },
    .{ .prefix = "secure_getenv(", .pattern = .quoted },
};

const dotnet = [_]Accessor{
    .{ .prefix = "Environment.GetEnvironmentVariable(", .pattern = .quoted },
    .{ .prefix = "System.Environment.GetEnvironmentVariable(", .pattern = .quoted },
};

const visualbasic = [_]Accessor{
    .{ .prefix = "Environment.GetEnvironmentVariable(", .pattern = .quoted },
    .{ .prefix = "Environ(", .pattern = .quoted },
};

const php = [_]Accessor{
    .{ .prefix = "getenv(", .pattern = .quoted },
    .{ .prefix = "$_ENV[", .pattern = .quoted },
    .{ .prefix = "$_SERVER[", .pattern = .quoted },
};

const perl = [_]Accessor{
    .{ .prefix = "$ENV{", .pattern = .braced },
};

const powershell = [_]Accessor{
    .{ .prefix = "$env:", .pattern = .ident },
    .{ .prefix = "[Environment]::GetEnvironmentVariable(", .pattern = .quoted },
    .{ .prefix = "[System.Environment]::GetEnvironmentVariable(", .pattern = .quoted },
};

const swift = [_]Accessor{
    .{ .prefix = "ProcessInfo.processInfo.environment[", .pattern = .quoted },
    .{ .prefix = "getenv(", .pattern = .quoted },
};

const objc = [_]Accessor{
    .{ .prefix = "getenv(", .pattern = .quoted },
    .{ .prefix = "environment] objectForKey:@", .pattern = .quoted },
};

const dart = [_]Accessor{
    .{ .prefix = "Platform.environment[", .pattern = .quoted },
};

const elixir = [_]Accessor{
    .{ .prefix = "System.get_env(", .pattern = .quoted },
    .{ .prefix = "System.fetch_env!(", .pattern = .quoted },
    .{ .prefix = "System.fetch_env(", .pattern = .quoted },
};

const erlang = [_]Accessor{
    .{ .prefix = "os:getenv(", .pattern = .quoted },
};

const haskell = [_]Accessor{
    .{ .prefix = "getEnv ", .pattern = .quoted },
    .{ .prefix = "lookupEnv ", .pattern = .quoted },
};

const ocaml = [_]Accessor{
    .{ .prefix = "Sys.getenv ", .pattern = .quoted },
    .{ .prefix = "Sys.getenv_opt ", .pattern = .quoted },
    .{ .prefix = "Unix.getenv ", .pattern = .quoted },
};

const lua = [_]Accessor{
    .{ .prefix = "os.getenv(", .pattern = .quoted },
};

const r = [_]Accessor{
    .{ .prefix = "Sys.getenv(", .pattern = .quoted },
};

const julia = [_]Accessor{
    .{ .prefix = "ENV[", .pattern = .quoted },
    .{ .prefix = "get(ENV, ", .pattern = .quoted },
};

const crystal = [_]Accessor{
    .{ .prefix = "ENV[", .pattern = .quoted },
    .{ .prefix = "ENV.fetch(", .pattern = .quoted },
};

const nim = [_]Accessor{
    .{ .prefix = "getEnv(", .pattern = .quoted },
    .{ .prefix = "existsEnv(", .pattern = .quoted },
};

const dlang = [_]Accessor{
    .{ .prefix = "environment[", .pattern = .quoted },
    .{ .prefix = "environment.get(", .pattern = .quoted },
    // NOTE: `"FOO" in environment` puts the key BEFORE the prefix; a forward
    // prefix matcher cannot catch it. Accept the miss or special-case it.
};

const vlang = [_]Accessor{
    .{ .prefix = "os.getenv(", .pattern = .quoted },
};

const fortran = [_]Accessor{
    .{ .prefix = "get_environment_variable(", .pattern = .quoted },
};

const pascal = [_]Accessor{
    .{ .prefix = "GetEnvironmentVariable(", .pattern = .quoted },
    .{ .prefix = "GetEnv(", .pattern = .quoted },
};

const tcl = [_]Accessor{
    .{ .prefix = "$env(", .pattern = .parened },
};

const nushell = [_]Accessor{
    .{ .prefix = "$env.", .pattern = .ident },
};

// const shell = [_]Accessor{
//     .{ .prefix = "${", .pattern = .braced },
//     .{ .prefix = "$", .pattern = .ident },
// };

pub const extensions = std.StaticStringMap([]const Accessor).initComptime(.{
    .{ "js", &javascript },   .{ "mjs", &javascript }, .{ "cjs", &javascript },
    .{ "jsx", &javascript },  .{ "ts", &javascript },  .{ "tsx", &javascript },
    .{ "mts", &javascript },  .{ "cts", &javascript }, .{ "py", &python },
    .{ "pyi", &python },      .{ "pyw", &python },     .{ "go", &go },
    .{ "rs", &rust },         .{ "rb", &ruby },        .{ "rake", &ruby },
    .{ "gemspec", &ruby },    .{ "zig", &zig },        .{ "java", &java },
    .{ "kt", &java },         .{ "kts", &java },       .{ "scala", &scala },
    .{ "sc", &scala },        .{ "groovy", &java },    .{ "gradle", &java },
    .{ "clj", &clojure },     .{ "cljs", &clojure },   .{ "cljc", &clojure },
    .{ "c", &c },             .{ "cpp", &cpp },        .{ "cc", &cpp },
    .{ "cxx", &cpp },         .{ "hpp", &cpp },        .{ "hh", &cpp },
    .{ "hxx", &cpp },         .{ "h", &cpp },          .{ "m", &objc },
    .{ "mm", &objc },         .{ "cs", &dotnet },      .{ "fs", &dotnet },
    .{ "fsx", &dotnet },      .{ "fsi", &dotnet },     .{ "vb", &visualbasic },
    .{ "php", &php },         .{ "pl", &perl },        .{ "pm", &perl },
    .{ "t", &perl },          .{ "ps1", &powershell }, .{ "psm1", &powershell },
    .{ "psd1", &powershell }, .{ "swift", &swift },    .{ "dart", &dart },
    .{ "ex", &elixir },       .{ "exs", &elixir },     .{ "erl", &erlang },
    .{ "hrl", &erlang },      .{ "hs", &haskell },     .{ "lhs", &haskell },
    .{ "ml", &ocaml },        .{ "mli", &ocaml },      .{ "lua", &lua },
    .{ "r", &r },             .{ "jl", &julia },       .{ "cr", &crystal },
    .{ "nim", &nim },         .{ "d", &dlang },        .{ "v", &vlang },
    .{ "f90", &fortran },     .{ "f95", &fortran },    .{ "f03", &fortran },
    .{ "f08", &fortran },     .{ "for", &fortran },    .{ "f", &fortran },
    .{ "pas", &pascal },      .{ "pp", &pascal },      .{ "dpr", &pascal },
    .{ "tcl", &tcl },         .{ "nu", &nushell },
    // .{ "sh", &shell }, .{ "bash", &shell }, .{ "zsh", &shell }, .{ "ksh", &shell },
});
