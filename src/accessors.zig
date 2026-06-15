const std = @import("std");

// Env-variable access patterns, grouped by language and keyed by file
// extension. The scanner matches `prefix` at a position, then extracts the key
// using `extract`. This replaces convention-based suffix matching (`_ENV`) with
// intent-based matching: a token is an env var because it is read through a
// known environment accessor, not because of how it is spelled.
//
// WHAT THIS CANNOT CATCH (true of every language, lexical limit):
//   - destructuring:   const { DATABASE_URL } = process.env
//   - aliasing:        const e = process.env; e.FOO
//   - dynamic keys:    process.env[name]        os.getenv(var)

pub const Extract = enum {
    ident, // bareword identifier follows the prefix:        process.env.FOO
    quoted, // first string literal ("..."/'...') follows:    getenv("FOO")
    braced, // token (bareword or quoted) up to '}':          $ENV{FOO}  ${FOO}
    parened, // bareword up to ')':                            $env(FOO)
};

pub const Accessor = struct {
    prefix: []const u8,
    extract: Extract,
};

const javascript = [_]Accessor{
    .{ .prefix = "process.env.", .extract = .ident },
    .{ .prefix = "process.env[", .extract = .quoted },
    .{ .prefix = "import.meta.env.", .extract = .ident },
    .{ .prefix = "import.meta.env[", .extract = .quoted },
};

const deno_bun = [_]Accessor{
    .{ .prefix = "Deno.env.get(", .extract = .quoted },
    .{ .prefix = "Deno.env.has(", .extract = .quoted },
    .{ .prefix = "Bun.env.", .extract = .ident },
    .{ .prefix = "Bun.env[", .extract = .quoted },
};

const python = [_]Accessor{
    .{ .prefix = "os.getenv(", .extract = .quoted },
    .{ .prefix = "os.environ[", .extract = .quoted },
    .{ .prefix = "os.environ.get(", .extract = .quoted },
    .{ .prefix = "os.environ.setdefault(", .extract = .quoted },
    .{ .prefix = "getenv(", .extract = .quoted },
    .{ .prefix = "environ[", .extract = .quoted },
    .{ .prefix = "environ.get(", .extract = .quoted },
};

const go = [_]Accessor{
    .{ .prefix = "os.Getenv(", .extract = .quoted },
    .{ .prefix = "os.LookupEnv(", .extract = .quoted },
};

const rust = [_]Accessor{
    .{ .prefix = "std::env::var(", .extract = .quoted },
    .{ .prefix = "std::env::var_os(", .extract = .quoted },
    .{ .prefix = "env::var(", .extract = .quoted },
    .{ .prefix = "env::var_os(", .extract = .quoted },
    .{ .prefix = "env!(", .extract = .quoted },
    .{ .prefix = "option_env!(", .extract = .quoted },
};

const ruby = [_]Accessor{
    .{ .prefix = "ENV[", .extract = .quoted },
    .{ .prefix = "ENV.fetch(", .extract = .quoted },
};

const zig = [_]Accessor{
    .{ .prefix = "std.posix.getenv(", .extract = .quoted },
    .{ .prefix = "posix.getenv(", .extract = .quoted },
    .{ .prefix = "std.process.getEnvVarOwned(", .extract = .quoted },
    .{ .prefix = "std.process.hasEnvVar(", .extract = .quoted },
    .{ .prefix = "std.process.hasEnvVarConstant(", .extract = .quoted },
    .{ .prefix = "std.process.parseEnvVarInt(", .extract = .quoted },
};

const java = [_]Accessor{
    .{ .prefix = "System.getenv(", .extract = .quoted },
};

const scala = [_]Accessor{
    .{ .prefix = "sys.env(", .extract = .quoted },
    .{ .prefix = "sys.env.get(", .extract = .quoted },
    .{ .prefix = "System.getenv(", .extract = .quoted },
};

const clojure = [_]Accessor{
    .{ .prefix = "System/getenv ", .extract = .quoted },
    .{ .prefix = "(getenv ", .extract = .quoted },
};

const c = [_]Accessor{
    .{ .prefix = "getenv(", .extract = .quoted },
    .{ .prefix = "secure_getenv(", .extract = .quoted },
};

const cpp = [_]Accessor{
    .{ .prefix = "std::getenv(", .extract = .quoted },
    .{ .prefix = "getenv(", .extract = .quoted },
    .{ .prefix = "secure_getenv(", .extract = .quoted },
};

const dotnet = [_]Accessor{
    .{ .prefix = "Environment.GetEnvironmentVariable(", .extract = .quoted },
    .{ .prefix = "System.Environment.GetEnvironmentVariable(", .extract = .quoted },
};

const visualbasic = [_]Accessor{
    .{ .prefix = "Environment.GetEnvironmentVariable(", .extract = .quoted },
    .{ .prefix = "Environ(", .extract = .quoted },
};

const php = [_]Accessor{
    .{ .prefix = "getenv(", .extract = .quoted },
    .{ .prefix = "$_ENV[", .extract = .quoted },
    .{ .prefix = "$_SERVER[", .extract = .quoted },
};

const perl = [_]Accessor{
    .{ .prefix = "$ENV{", .extract = .braced },
};

const powershell = [_]Accessor{
    .{ .prefix = "$env:", .extract = .ident },
    .{ .prefix = "[Environment]::GetEnvironmentVariable(", .extract = .quoted },
    .{ .prefix = "[System.Environment]::GetEnvironmentVariable(", .extract = .quoted },
};

const swift = [_]Accessor{
    .{ .prefix = "ProcessInfo.processInfo.environment[", .extract = .quoted },
    .{ .prefix = "getenv(", .extract = .quoted },
};

const objc = [_]Accessor{
    .{ .prefix = "getenv(", .extract = .quoted },
    .{ .prefix = "environment] objectForKey:@", .extract = .quoted },
};

const dart = [_]Accessor{
    .{ .prefix = "Platform.environment[", .extract = .quoted },
};

const elixir = [_]Accessor{
    .{ .prefix = "System.get_env(", .extract = .quoted },
    .{ .prefix = "System.fetch_env!(", .extract = .quoted },
    .{ .prefix = "System.fetch_env(", .extract = .quoted },
};

const erlang = [_]Accessor{
    .{ .prefix = "os:getenv(", .extract = .quoted },
};

const haskell = [_]Accessor{
    .{ .prefix = "getEnv ", .extract = .quoted },
    .{ .prefix = "lookupEnv ", .extract = .quoted },
};

const ocaml = [_]Accessor{
    .{ .prefix = "Sys.getenv ", .extract = .quoted },
    .{ .prefix = "Sys.getenv_opt ", .extract = .quoted },
    .{ .prefix = "Unix.getenv ", .extract = .quoted },
};

const lua = [_]Accessor{
    .{ .prefix = "os.getenv(", .extract = .quoted },
};

const r = [_]Accessor{
    .{ .prefix = "Sys.getenv(", .extract = .quoted },
};

const julia = [_]Accessor{
    .{ .prefix = "ENV[", .extract = .quoted },
    .{ .prefix = "get(ENV, ", .extract = .quoted },
};

const crystal = [_]Accessor{
    .{ .prefix = "ENV[", .extract = .quoted },
    .{ .prefix = "ENV.fetch(", .extract = .quoted },
};

const nim = [_]Accessor{
    .{ .prefix = "getEnv(", .extract = .quoted },
    .{ .prefix = "existsEnv(", .extract = .quoted },
};

const dlang = [_]Accessor{
    .{ .prefix = "environment[", .extract = .quoted },
    .{ .prefix = "environment.get(", .extract = .quoted },
    // NOTE: `"FOO" in environment` puts the key BEFORE the prefix; a forward
    // prefix matcher cannot catch it. Accept the miss or special-case it.
};

const vlang = [_]Accessor{
    .{ .prefix = "os.getenv(", .extract = .quoted },
};

const fortran = [_]Accessor{
    .{ .prefix = "get_environment_variable(", .extract = .quoted },
};

const pascal = [_]Accessor{
    .{ .prefix = "GetEnvironmentVariable(", .extract = .quoted },
    .{ .prefix = "GetEnv(", .extract = .quoted },
};

const tcl = [_]Accessor{
    .{ .prefix = "$env(", .extract = .parened },
};

const nushell = [_]Accessor{
    .{ .prefix = "$env.", .extract = .ident },
};

// const shell = [_]Accessor{
//     .{ .prefix = "${", .extract = .braced },
//     .{ .prefix = "$", .extract = .ident },
// };

// ----------------------------------------------------------------------------
// Extension -> accessor table
// ----------------------------------------------------------------------------
pub const by_ext = std.StaticStringMap([]const Accessor).initComptime(.{
    .{ "js", &javascript },   .{ "mjs", &javascript },  .{ "cjs", &javascript },
    .{ "jsx", &javascript },  .{ "ts", &javascript },   .{ "tsx", &javascript },
    .{ "mts", &javascript },  .{ "cts", &javascript },  .{ "py", &python },
    .{ "pyi", &python },      .{ "pyw", &python },      .{ "go", &go },
    .{ "rs", &rust },         .{ "rb", &ruby },         .{ "rake", &ruby },
    .{ "gemspec", &ruby },    .{ "zig", &zig },         .{ "java", &java },
    .{ "kt", &java },         .{ "kts", &java },        .{ "scala", &scala },
    .{ "sc", &scala },        .{ "groovy", &java },     .{ "gradle", &java },
    .{ "cr", &crystal },      .{ "clj", &clojure },     .{ "cljs", &clojure },
    .{ "cljc", &clojure },    .{ "c", &c },             .{ "cpp", &cpp },
    .{ "cc", &cpp },          .{ "cxx", &cpp },         .{ "hpp", &cpp },
    .{ "hh", &cpp },          .{ "hxx", &cpp },         .{ "h", &cpp },
    .{ "m", &objc },          .{ "mm", &objc },         .{ "cs", &dotnet },
    .{ "fs", &dotnet },       .{ "fsx", &dotnet },      .{ "fsi", &dotnet },
    .{ "vb", &visualbasic },  .{ "php", &php },         .{ "pl", &perl },
    .{ "pm", &perl },         .{ "t", &perl },          .{ "ps1", &powershell },
    .{ "psm1", &powershell }, .{ "psd1", &powershell }, .{ "swift", &swift },
    .{ "dart", &dart },       .{ "ex", &elixir },       .{ "exs", &elixir },
    .{ "erl", &erlang },      .{ "hrl", &erlang },      .{ "hs", &haskell },
    .{ "lhs", &haskell },     .{ "ml", &ocaml },        .{ "mli", &ocaml },
    .{ "lua", &lua },         .{ "r", &r },             .{ "jl", &julia },
    .{ "cr", &crystal },      .{ "nim", &nim },         .{ "d", &dlang },
    .{ "v", &vlang },         .{ "f90", &fortran },     .{ "f95", &fortran },
    .{ "f03", &fortran },     .{ "f08", &fortran },     .{ "for", &fortran },
    .{ "f", &fortran },       .{ "pas", &pascal },      .{ "pp", &pascal },
    .{ "dpr", &pascal },      .{ "tcl", &tcl },         .{ "nu", &nushell },
    // .{ "sh", &shell }, .{ "bash", &shell }, .{ "zsh", &shell }, .{ "ksh", &shell },
});

pub fn forExt(ext: []const u8) ?[]const Accessor {
    return by_ext.get(ext);
}
