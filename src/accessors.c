#include "accessors.h"
#include "macros.h"
#include <string.h>

#define ENTRY(ext, arr)                                                                                                \
    { (ext), (arr), ARR_LEN(arr) }

static const accessor_t javascript[] = {
    {.prefix = "process.env.", .prefix_len = sizeof("process.env.") - 1, .pattern = ident},
    {.prefix = "process.env[", .prefix_len = sizeof("process.env[") - 1, .pattern = quoted},
    {.prefix = "import.meta.env.", .prefix_len = sizeof("import.meta.env.") - 1, .pattern = ident},
    {.prefix = "import.meta.env[", .prefix_len = sizeof("import.meta.env[") - 1, .pattern = quoted},
    {.prefix = "Deno.env.get(", .prefix_len = sizeof("Deno.env.get(") - 1, .pattern = quoted},
    {.prefix = "Bun.env.", .prefix_len = sizeof("Bun.env.") - 1, .pattern = ident},
    {.prefix = "Bun.env[", .prefix_len = sizeof("Bun.env[") - 1, .pattern = quoted},
};

static const accessor_t python[] = {
    {.prefix = "os.getenv(", .prefix_len = sizeof("os.getenv(") - 1, .pattern = quoted},
    {.prefix = "os.environ[", .prefix_len = sizeof("os.environ[") - 1, .pattern = quoted},
    {.prefix = "os.environ.get(", .prefix_len = sizeof("os.environ.get(") - 1, .pattern = quoted},
    {.prefix = "os.environ.setdefault(", .prefix_len = sizeof("os.environ.setdefault(") - 1, .pattern = quoted},
    {.prefix = "getenv(", .prefix_len = sizeof("getenv(") - 1, .pattern = quoted},
    {.prefix = "environ[", .prefix_len = sizeof("environ[") - 1, .pattern = quoted},
    {.prefix = "environ.get(", .prefix_len = sizeof("environ.get(") - 1, .pattern = quoted},
};

static const accessor_t go[] = {
    {.prefix = "os.Getenv(", .prefix_len = sizeof("os.Getenv(") - 1, .pattern = quoted},
    {.prefix = "os.LookupEnv(", .prefix_len = sizeof("os.LookupEnv(") - 1, .pattern = quoted},
};

static const accessor_t rust[] = {
    {.prefix = "std::env::var(", .prefix_len = sizeof("std::env::var(") - 1, .pattern = quoted},
    {.prefix = "std::env::var_os(", .prefix_len = sizeof("std::env::var_os(") - 1, .pattern = quoted},
    {.prefix = "env::var(", .prefix_len = sizeof("env::var(") - 1, .pattern = quoted},
    {.prefix = "env::var_os(", .prefix_len = sizeof("env::var_os(") - 1, .pattern = quoted},
    {.prefix = "env!(", .prefix_len = sizeof("env!(") - 1, .pattern = quoted},
    {.prefix = "option_env!(", .prefix_len = sizeof("option_env!(") - 1, .pattern = quoted},
};

static const accessor_t ruby[] = {
    {.prefix = "ENV[", .prefix_len = sizeof("ENV[") - 1, .pattern = quoted},
    {.prefix = "ENV.fetch(", .prefix_len = sizeof("ENV.fetch(") - 1, .pattern = quoted},
};

static const accessor_t zig[] = {
    {.prefix = "std.posix.getenv(", .prefix_len = sizeof("std.posix.getenv(") - 1, .pattern = quoted},
    {.prefix = "posix.getenv(", .prefix_len = sizeof("posix.getenv(") - 1, .pattern = quoted},
    {.prefix = "std.process.getEnvVarOwned(",
     .prefix_len = sizeof("std.process.getEnvVarOwned(") - 1,
     .pattern = quoted},
    {.prefix = "std.process.hasEnvVar(", .prefix_len = sizeof("std.process.hasEnvVar(") - 1, .pattern = quoted},
    {.prefix = "std.process.hasEnvVarConstant(",
     .prefix_len = sizeof("std.process.hasEnvVarConstant(") - 1,
     .pattern = quoted},
    {.prefix = "std.process.parseEnvVarInt(",
     .prefix_len = sizeof("std.process.parseEnvVarInt(") - 1,
     .pattern = quoted},
};

static const accessor_t java[] = {
    {.prefix = "System.getenv(", .prefix_len = sizeof("System.getenv(") - 1, .pattern = quoted},
};

static const accessor_t scala[] = {
    {.prefix = "sys.env(", .prefix_len = sizeof("sys.env(") - 1, .pattern = quoted},
    {.prefix = "sys.env.get(", .prefix_len = sizeof("sys.env.get(") - 1, .pattern = quoted},
    {.prefix = "System.getenv(", .prefix_len = sizeof("System.getenv(") - 1, .pattern = quoted},
};

static const accessor_t clojure[] = {
    {.prefix = "System/getenv ", .prefix_len = sizeof("System/getenv ") - 1, .pattern = quoted},
    {.prefix = "(getenv ", .prefix_len = sizeof("(getenv ") - 1, .pattern = quoted},
};

static const accessor_t c[] = {
    {.prefix = "getenv(", .prefix_len = sizeof("getenv(") - 1, .pattern = quoted},
    {.prefix = "secure_getenv(", .prefix_len = sizeof("secure_getenv(") - 1, .pattern = quoted},
};

static const accessor_t cpp[] = {
    {.prefix = "std::getenv(", .prefix_len = sizeof("std::getenv(") - 1, .pattern = quoted},
    {.prefix = "getenv(", .prefix_len = sizeof("getenv(") - 1, .pattern = quoted},
    {.prefix = "secure_getenv(", .prefix_len = sizeof("secure_getenv(") - 1, .pattern = quoted},
};

static const accessor_t dotnet[] = {
    {.prefix = "Environment.GetEnvironmentVariable(",
     .prefix_len = sizeof("Environment.GetEnvironmentVariable(") - 1,
     .pattern = quoted},
    {.prefix = "System.Environment.GetEnvironmentVariable(",
     .prefix_len = sizeof("System.Environment.GetEnvironmentVariable(") - 1,
     .pattern = quoted},
};

static const accessor_t visualbasic[] = {
    {.prefix = "Environment.GetEnvironmentVariable(",
     .prefix_len = sizeof("Environment.GetEnvironmentVariable(") - 1,
     .pattern = quoted},
    {.prefix = "Environ(", .prefix_len = sizeof("Environ(") - 1, .pattern = quoted},
};

static const accessor_t php[] = {
    {.prefix = "getenv(", .prefix_len = sizeof("getenv(") - 1, .pattern = quoted},
    {.prefix = "$_ENV[", .prefix_len = sizeof("$_ENV[") - 1, .pattern = quoted},
    {.prefix = "$_SERVER[", .prefix_len = sizeof("$_SERVER[") - 1, .pattern = quoted},
};

static const accessor_t perl[] = {
    {.prefix = "$ENV{", .prefix_len = sizeof("$ENV{") - 1, .pattern = braced},
};

static const accessor_t powershell[] = {
    {.prefix = "$env:", .prefix_len = sizeof("$env:") - 1, .pattern = ident},
    {.prefix = "[Environment]::GetEnvironmentVariable(",
     .prefix_len = sizeof("[Environment]::GetEnvironmentVariable(") - 1,
     .pattern = quoted},
    {.prefix = "[System.Environment]::GetEnvironmentVariable(",
     .prefix_len = sizeof("[System.Environment]::GetEnvironmentVariable(") - 1,
     .pattern = quoted},
};

static const accessor_t swift[] = {
    {.prefix = "ProcessInfo.processInfo.environment[",
     .prefix_len = sizeof("ProcessInfo.processInfo.environment[") - 1,
     .pattern = quoted},
    {.prefix = "getenv(", .prefix_len = sizeof("getenv(") - 1, .pattern = quoted},
};

static const accessor_t objc[] = {
    {.prefix = "getenv(", .prefix_len = sizeof("getenv(") - 1, .pattern = quoted},
    {.prefix = "environment] objectForKey:@",
     .prefix_len = sizeof("environment] objectForKey:@") - 1,
     .pattern = quoted},
};

static const accessor_t dart[] = {
    {.prefix = "Platform.environment[", .prefix_len = sizeof("Platform.environment[") - 1, .pattern = quoted},
};

static const accessor_t elixir[] = {
    {.prefix = "System.get_env(", .prefix_len = sizeof("System.get_env(") - 1, .pattern = quoted},
    {.prefix = "System.fetch_env!(", .prefix_len = sizeof("System.fetch_env!(") - 1, .pattern = quoted},
    {.prefix = "System.fetch_env(", .prefix_len = sizeof("System.fetch_env(") - 1, .pattern = quoted},
};

static const accessor_t erlang[] = {
    {.prefix = "os:getenv(", .prefix_len = sizeof("os:getenv(") - 1, .pattern = quoted},
};

static const accessor_t haskell[] = {
    {.prefix = "getEnv ", .prefix_len = sizeof("getEnv ") - 1, .pattern = quoted},
    {.prefix = "lookupEnv ", .prefix_len = sizeof("lookupEnv ") - 1, .pattern = quoted},
};

static const accessor_t ocaml[] = {
    {.prefix = "Sys.getenv ", .prefix_len = sizeof("Sys.getenv ") - 1, .pattern = quoted},
    {.prefix = "Sys.getenv_opt ", .prefix_len = sizeof("Sys.getenv_opt ") - 1, .pattern = quoted},
    {.prefix = "Unix.getenv ", .prefix_len = sizeof("Unix.getenv ") - 1, .pattern = quoted},
};

static const accessor_t lua[] = {
    {.prefix = "os.getenv(", .prefix_len = sizeof("os.getenv(") - 1, .pattern = quoted},
};

static const accessor_t r[] = {
    {.prefix = "Sys.getenv(", .prefix_len = sizeof("Sys.getenv(") - 1, .pattern = quoted},
};

static const accessor_t julia[] = {
    {.prefix = "ENV[", .prefix_len = sizeof("ENV[") - 1, .pattern = quoted},
    {.prefix = "get(ENV, ", .prefix_len = sizeof("get(ENV, ") - 1, .pattern = quoted},
};

static const accessor_t crystal[] = {
    {.prefix = "ENV[", .prefix_len = sizeof("ENV[") - 1, .pattern = quoted},
    {.prefix = "ENV.fetch(", .prefix_len = sizeof("ENV.fetch(") - 1, .pattern = quoted},
};

static const accessor_t nim[] = {
    {.prefix = "getEnv(", .prefix_len = sizeof("getEnv(") - 1, .pattern = quoted},
    {.prefix = "existsEnv(", .prefix_len = sizeof("existsEnv(") - 1, .pattern = quoted},
};

static const accessor_t dlang[] = {
    {.prefix = "environment[", .prefix_len = sizeof("environment[") - 1, .pattern = quoted},
    {.prefix = "environment.get(", .prefix_len = sizeof("environment.get(") - 1, .pattern = quoted},
    // NOTE: `"FOO" in environment` puts the key BEFORE the prefix; a forward
    // prefix matcher cannot catch it. Accept the miss or special-case it.
};

static const accessor_t vlang[] = {
    {.prefix = "os.getenv(", .prefix_len = sizeof("os.getenv(") - 1, .pattern = quoted},
};

static const accessor_t fortran[] = {
    {.prefix = "get_environment_variable(", .prefix_len = sizeof("get_environment_variable(") - 1, .pattern = quoted},
};

static const accessor_t pascal[] = {
    {.prefix = "GetEnvironmentVariable(", .prefix_len = sizeof("GetEnvironmentVariable(") - 1, .pattern = quoted},
    {.prefix = "GetEnv(", .prefix_len = sizeof("GetEnv(") - 1, .pattern = quoted},
};

static const accessor_t tcl[] = {
    {.prefix = "$env(", .prefix_len = sizeof("$env(") - 1, .pattern = parened},
};

static const accessor_t nushell[] = {
    {.prefix = "$env.", .prefix_len = sizeof("$env.") - 1, .pattern = ident},
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
    for (size_t i = 0; i < ARR_LEN(extensions); ++i) {
        if (strcmp(ext, extensions[i].ext) == 0) {
            return &extensions[i];
        }
    }
    return NULL;
}

const file_ext_t *get_file_ext(const file_ext_map_t *map, const char *ext) {
    for (size_t i = 0; i < map->count; ++i) {
        if (strcmp(map->items[i].ext, ext) == 0) {
            return &map->items[i];
        }
    }
    return NULL;
}

void file_ext_append(file_ext_map_t *map, const ext_entry *entry) {
    if (get_file_ext(map, entry->ext) != NULL) {
        return;
    }

    file_ext_t match = {
        .ext = entry->ext,
        .accessors = entry->accessors,
        .accessor_count = entry->count,
    };

    DYN_ARR_APPEND(map, match);
}
