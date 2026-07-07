#include "accessors.h"
#include "macros.h"
#include <string.h>

static const accessor_t javascript[] = {
    ACCESSOR("process.env.", ident),      ACCESSOR("process.env[", quoted),  ACCESSOR("import.meta.env.", ident),
    ACCESSOR("import.meta.env[", quoted), ACCESSOR("Deno.env.get(", quoted), ACCESSOR("Bun.env.", ident),
    ACCESSOR("Bun.env[", quoted),
};

static const accessor_t python[] = {
    ACCESSOR("os.environ.setdefault(", quoted),
    ACCESSOR("getenv(", quoted),
    ACCESSOR("environ[", quoted),
    ACCESSOR("environ.get(", quoted),
};

static const accessor_t go[] = {
    ACCESSOR("os.Getenv(", quoted),
    ACCESSOR("os.LookupEnv(", quoted),
};

static const accessor_t rust[] = {
    ACCESSOR("env::var(", quoted),
    ACCESSOR("env::var_os(", quoted),
    ACCESSOR("env!(", quoted),
    ACCESSOR("option_env!(", quoted),
};

static const accessor_t ruby[] = {
    ACCESSOR("ENV[", quoted),
    ACCESSOR("ENV.fetch(", quoted),
};

static const accessor_t zig[] = {
    ACCESSOR("getenv(", quoted),         ACCESSOR("getEnvVarOwned(", quoted),
    ACCESSOR("hasEnvVar(", quoted),      ACCESSOR("hasEnvVarConstant(", quoted),
    ACCESSOR("parseEnvVarInt(", quoted),
};

static const accessor_t java[] = {
    ACCESSOR("System.getenv(", quoted),
};

static const accessor_t scala[] = {
    ACCESSOR("sys.env(", quoted),
    ACCESSOR("sys.env.get(", quoted),
    ACCESSOR("System.getenv(", quoted),
};

static const accessor_t clojure[] = {
    ACCESSOR("System/getenv ", quoted),
    ACCESSOR("(getenv ", quoted),
};

static const accessor_t c[] = {
    ACCESSOR("getenv(", quoted),
    ACCESSOR("secure_getenv(", quoted),
};

static const accessor_t cpp[] = {
    ACCESSOR("getenv(", quoted),
};

static const accessor_t dotnet[] = {
    ACCESSOR("Environment.GetEnvironmentVariable(", quoted),
};

static const accessor_t visualbasic[] = {
    ACCESSOR("Environment.GetEnvironmentVariable(", quoted),
    ACCESSOR("Environ(", quoted),
};

static const accessor_t php[] = {
    ACCESSOR("getenv(", quoted),
    ACCESSOR("$_ENV[", quoted),
    ACCESSOR("$_SERVER[", quoted),
};

static const accessor_t perl[] = {
    ACCESSOR("$ENV{", braced),
};

static const accessor_t powershell[] = {
    ACCESSOR("$env:", ident),
    ACCESSOR("[Environment]::GetEnvironmentVariable(", quoted),
    ACCESSOR("[System.Environment]::GetEnvironmentVariable(", quoted),
};

static const accessor_t swift[] = {
    ACCESSOR("ProcessInfo.processInfo.environment[", quoted),
    ACCESSOR("getenv(", quoted),
};

static const accessor_t objc[] = {
    ACCESSOR("getenv(", quoted),
    ACCESSOR("environment] objectForKey:@", quoted),
};

static const accessor_t dart[] = {
    ACCESSOR("Platform.environment[", quoted),
};

static const accessor_t elixir[] = {
    ACCESSOR("System.get_env(", quoted),
    ACCESSOR("System.fetch_env!(", quoted),
    ACCESSOR("System.fetch_env(", quoted),
};

static const accessor_t erlang[] = {
    ACCESSOR("os:getenv(", quoted),
};

static const accessor_t haskell[] = {
    ACCESSOR("getEnv ", quoted),
    ACCESSOR("lookupEnv ", quoted),
};

static const accessor_t ocaml[] = {
    ACCESSOR("Sys.getenv ", quoted),
    ACCESSOR("Sys.getenv_opt ", quoted),
    ACCESSOR("Unix.getenv ", quoted),
};

static const accessor_t lua[] = {
    ACCESSOR("os.getenv(", quoted),
};

static const accessor_t r[] = {
    ACCESSOR("Sys.getenv(", quoted),
};

static const accessor_t julia[] = {
    ACCESSOR("ENV[", quoted),
    ACCESSOR("get(ENV, ", quoted),
};

static const accessor_t crystal[] = {
    ACCESSOR("ENV[", quoted),
    ACCESSOR("ENV.fetch(", quoted),
};

static const accessor_t nim[] = {
    ACCESSOR("getEnv(", quoted),
    ACCESSOR("existsEnv(", quoted),
};

static const accessor_t dlang[] = {
    ACCESSOR("environment[", quoted),
    ACCESSOR("environment.get(", quoted),
};

static const accessor_t vlang[] = {
    ACCESSOR("os.getenv(", quoted),
};

static const accessor_t fortran[] = {
    ACCESSOR("get_environment_variable(", quoted),
};

static const accessor_t pascal[] = {
    ACCESSOR("GetEnvironmentVariable(", quoted),
    ACCESSOR("GetEnv(", quoted),
};

static const accessor_t tcl[] = {
    ACCESSOR("$env(", parened),
};

static const accessor_t nushell[] = {
    ACCESSOR("$env.", ident),
};

static const accessor_t yaml[] = {
    ACCESSOR("${", expansion),
};

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
    ENTRY("yaml", yaml),
    ENTRY("yml", yaml),
};

static const size_t ext_len = ARR_LEN(extensions);

const ext_entry *get_scan_extension(const char *ext) {
    for (size_t i = 0; i < ext_len; ++i) {
        if (strcmp(ext, extensions[i].ext) == 0) {
            return &extensions[i];
        }
    }
    return NULL;
}

const file_ext_t *get_file_extension(const file_ext_map_t *map, const char *ext) {
    for (size_t i = 0; i < map->count; ++i) {
        if (strcmp(map->items[i].ext, ext) == 0) {
            return &map->items[i];
        }
    }
    return NULL;
}

void append_file_extension(file_ext_map_t *map, const ext_entry *entry) {
    if (get_file_extension(map, entry->ext) != NULL) {
        return;
    }

    file_ext_t match = {
        .ext = entry->ext,
        .accessors = entry->accessors,
        .accessor_count = entry->count,
    };

    DYN_ARR_APPEND(map, match);
}
