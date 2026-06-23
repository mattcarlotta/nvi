#include "accessors.h"
#include <string.h>

#define ARRLEN(a) (sizeof(a) / sizeof((a)[0]))
#define ENTRY(ext, arr) {(ext), (arr), ARRLEN(arr)}

static const accessor_t javascript[] = {
    {.prefix = "process.env.", .pattern = ident},     {.prefix = "process.env[", .pattern = quoted},
    {.prefix = "import.meta.env.", .pattern = ident}, {.prefix = "import.meta.env[", .pattern = quoted},
    {.prefix = "Deno.env.get(", .pattern = quoted},   {.prefix = "Bun.env.", .pattern = ident},
    {.prefix = "Bun.env[", .pattern = quoted},
};

static const accessor_t python[] = {
    {.prefix = "os.getenv(", .pattern = quoted},      {.prefix = "os.environ[", .pattern = quoted},
    {.prefix = "os.environ.get(", .pattern = quoted}, {.prefix = "os.environ.setdefault(", .pattern = quoted},
    {.prefix = "getenv(", .pattern = quoted},         {.prefix = "environ[", .pattern = quoted},
    {.prefix = "environ.get(", .pattern = quoted},
};

static const accessor_t go[] = {
    {.prefix = "os.Getenv(", .pattern = quoted},
    {.prefix = "os.LookupEnv(", .pattern = quoted},
};

static const accessor_t rust[] = {
    {.prefix = "std::env::var(", .pattern = quoted}, {.prefix = "std::env::var_os(", .pattern = quoted},
    {.prefix = "env::var(", .pattern = quoted},      {.prefix = "env::var_os(", .pattern = quoted},
    {.prefix = "env!(", .pattern = quoted},          {.prefix = "option_env!(", .pattern = quoted},
};

static const accessor_t ruby[] = {
    {.prefix = "ENV[", .pattern = quoted},
    {.prefix = "ENV.fetch(", .pattern = quoted},
};

static const accessor_t zig[] = {
    {.prefix = "std.posix.getenv(", .pattern = quoted},
    {.prefix = "posix.getenv(", .pattern = quoted},
    {.prefix = "std.process.getEnvVarOwned(", .pattern = quoted},
    {.prefix = "std.process.hasEnvVar(", .pattern = quoted},
    {.prefix = "std.process.hasEnvVarConstant(", .pattern = quoted},
    {.prefix = "std.process.parseEnvVarInt(", .pattern = quoted},
};

static const accessor_t java[] = {
    {.prefix = "System.getenv(", .pattern = quoted},
};

static const accessor_t scala[] = {
    {.prefix = "sys.env(", .pattern = quoted},
    {.prefix = "sys.env.get(", .pattern = quoted},
    {.prefix = "System.getenv(", .pattern = quoted},
};

static const accessor_t clojure[] = {
    {.prefix = "System/getenv ", .pattern = quoted},
    {.prefix = "(getenv ", .pattern = quoted},
};

static const accessor_t c[] = {
    {.prefix = "getenv(", .pattern = quoted},
    {.prefix = "secure_getenv(", .pattern = quoted},
};

static const accessor_t cpp[] = {
    {.prefix = "std::getenv(", .pattern = quoted},
    {.prefix = "getenv(", .pattern = quoted},
    {.prefix = "secure_getenv(", .pattern = quoted},
};

static const accessor_t dotnet[] = {
    {.prefix = "Environment.GetEnvironmentVariable(", .pattern = quoted},
    {.prefix = "System.Environment.GetEnvironmentVariable(", .pattern = quoted},
};

static const accessor_t visualbasic[] = {
    {.prefix = "Environment.GetEnvironmentVariable(", .pattern = quoted},
    {.prefix = "Environ(", .pattern = quoted},
};

static const accessor_t php[] = {
    {.prefix = "getenv(", .pattern = quoted},
    {.prefix = "$_ENV[", .pattern = quoted},
    {.prefix = "$_SERVER[", .pattern = quoted},
};

static const accessor_t perl[] = {
    {.prefix = "$ENV{", .pattern = braced},
};

static const accessor_t powershell[] = {
    {.prefix = "$env:", .pattern = ident},
    {.prefix = "[Environment]::GetEnvironmentVariable(", .pattern = quoted},
    {.prefix = "[System.Environment]::GetEnvironmentVariable(", .pattern = quoted},
};

static const accessor_t swift[] = {
    {.prefix = "ProcessInfo.processInfo.environment[", .pattern = quoted},
    {.prefix = "getenv(", .pattern = quoted},
};

static const accessor_t objc[] = {
    {.prefix = "getenv(", .pattern = quoted},
    {.prefix = "environment] objectForKey:@", .pattern = quoted},
};

static const accessor_t dart[] = {
    {.prefix = "Platform.environment[", .pattern = quoted},
};

static const accessor_t elixir[] = {
    {.prefix = "System.get_env(", .pattern = quoted},
    {.prefix = "System.fetch_env!(", .pattern = quoted},
    {.prefix = "System.fetch_env(", .pattern = quoted},
};

static const accessor_t erlang[] = {
    {.prefix = "os:getenv(", .pattern = quoted},
};

static const accessor_t haskell[] = {
    {.prefix = "getEnv ", .pattern = quoted},
    {.prefix = "lookupEnv ", .pattern = quoted},
};

static const accessor_t ocaml[] = {
    {.prefix = "Sys.getenv ", .pattern = quoted},
    {.prefix = "Sys.getenv_opt ", .pattern = quoted},
    {.prefix = "Unix.getenv ", .pattern = quoted},
};

static const accessor_t lua[] = {
    {.prefix = "os.getenv(", .pattern = quoted},
};

static const accessor_t r[] = {
    {.prefix = "Sys.getenv(", .pattern = quoted},
};

static const accessor_t julia[] = {
    {.prefix = "ENV[", .pattern = quoted},
    {.prefix = "get(ENV, ", .pattern = quoted},
};

static const accessor_t crystal[] = {
    {.prefix = "ENV[", .pattern = quoted},
    {.prefix = "ENV.fetch(", .pattern = quoted},
};

static const accessor_t nim[] = {
    {.prefix = "getEnv(", .pattern = quoted},
    {.prefix = "existsEnv(", .pattern = quoted},
};

static const accessor_t dlang[] = {
    {.prefix = "environment[", .pattern = quoted},
    {.prefix = "environment.get(", .pattern = quoted},
    // NOTE: `"FOO" in environment` puts the key BEFORE the prefix; a forward
    // prefix matcher cannot catch it. Accept the miss or special-case it.
};

static const accessor_t vlang[] = {
    {.prefix = "os.getenv(", .pattern = quoted},
};

static const accessor_t fortran[] = {
    {.prefix = "get_environment_variable(", .pattern = quoted},
};

static const accessor_t pascal[] = {
    {.prefix = "GetEnvironmentVariable(", .pattern = quoted},
    {.prefix = "GetEnv(", .pattern = quoted},
};

static const accessor_t tcl[] = {
    {.prefix = "$env(", .pattern = parened},
};

static const accessor_t nushell[] = {
    {.prefix = "$env.", .pattern = ident},
};

// const shell = [_]Accessor{
//     .{ .prefix = "${", .pattern = .braced },
//     .{ .prefix = "$", .pattern = .ident },
// };

static const ext_entry extensions[] = {
    ENTRY("js", javascript),
    ENTRY("mjs", javascript),
    ENTRY("cjs", javascript),
    ENTRY("jsx", javascript),
    ENTRY("ts", javascript),
    ENTRY("tsx", javascript),
    ENTRY("mts", javascript),
    ENTRY("cts", javascript),
    ENTRY("py", python),
    ENTRY("pyi", python),
    ENTRY("pyw", python),
    ENTRY("go", go),
    ENTRY("rs", rust),
    ENTRY("rb", ruby),
    ENTRY("rake", ruby),
    ENTRY("gemspec", ruby),
    ENTRY("zig", zig),
    ENTRY("java", java),
    ENTRY("kt", java),
    ENTRY("kts", java),
    ENTRY("groovy", java),
    ENTRY("gradle", java),
    ENTRY("scala", scala),
    ENTRY("sc", scala),
    ENTRY("clj", clojure),
    ENTRY("cljs", clojure),
    ENTRY("cljc", clojure),
    ENTRY("c", c),
    ENTRY("cpp", cpp),
    ENTRY("cc", cpp),
    ENTRY("cxx", cpp),
    ENTRY("hpp", cpp),
    ENTRY("hh", cpp),
    ENTRY("hxx", cpp),
    ENTRY("h", cpp),
    ENTRY("m", objc),
    ENTRY("mm", objc),
    ENTRY("cs", dotnet),
    ENTRY("fs", dotnet),
    ENTRY("fsx", dotnet),
    ENTRY("fsi", dotnet),
    ENTRY("vb", visualbasic),
    ENTRY("php", php),
    ENTRY("pl", perl),
    ENTRY("pm", perl),
    ENTRY("t", perl),
    ENTRY("ps1", powershell),
    ENTRY("psm1", powershell),
    ENTRY("psd1", powershell),
    ENTRY("swift", swift),
    ENTRY("dart", dart),
    ENTRY("ex", elixir),
    ENTRY("exs", elixir),
    ENTRY("erl", erlang),
    ENTRY("hrl", erlang),
    ENTRY("hs", haskell),
    ENTRY("lhs", haskell),
    ENTRY("ml", ocaml),
    ENTRY("mli", ocaml),
    ENTRY("lua", lua),
    ENTRY("r", r),
    ENTRY("jl", julia),
    ENTRY("cr", crystal),
    ENTRY("nim", nim),
    ENTRY("d", dlang),
    ENTRY("v", vlang),
    ENTRY("f90", fortran),
    ENTRY("f95", fortran),
    ENTRY("f03", fortran),
    ENTRY("f08", fortran),
    ENTRY("for", fortran),
    ENTRY("f", fortran),
    ENTRY("pas", pascal),
    ENTRY("pp", pascal),
    ENTRY("dpr", pascal),
    ENTRY("tcl", tcl),
    ENTRY("nu", nushell),
};

const ext_entry *find_ext(const char *ext) {
    for (size_t i = 0; i < ARRLEN(extensions); i++) {
        if (strcmp(ext, extensions[i].ext) == 0) {
            return &extensions[i];
        }
    }
    return NULL;
}
