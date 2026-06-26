#include "info.h"
#include "version.h"
#include <stdio.h>

void log_help(void) {
    fputs(
        "Usage: nvi [flags] -- <command>\n"
        "       nvi scan [ext] [ext] ...etc (no <command>)\n"
        "\n"
        "Flags:\n"
        "  -d, --dry-run                      prints parsed flags, scan results, generated file tokens and the "
        "parsed ENVs to stderr\n"
        "  -f, --files <paths>                parses .env files in order (if running a <command>, then at least 1 .env "
        "file must be specified)\n"
        "  -F, --format <fmt>                 outputs ENV format (options: nul|powershell)\n"
        "  -h, --help, help                   prints this help and exits\n"
        "  -i, --ignored <keys>               ignores ENV keys that a scan may find and add to a required ENV key "
        "list\n"
        "  -r, --required <keys>              marks a list of ENV keys that must be present before the <command> is "
        "emitted\n"
        "  -s, --scan <ext>, scan <ext>       recursively scans in the root directory for ENV keys used within *.<ext> "
        "files and marks them as required \u2020\n"
        "  -v, --version, version             prints the version and exits\n"
        "\n"
        " \u2020 without a <command>, scan reports what it finds and exits; with a <command>, the found ENV keys are "
        "added to a required ENV key list\n"
        "\n"
        "Supported scan file extensions:\n"
        " \u2022 C -> c\n"
        " \u2022 Clojure -> clj, cljs, cljc\n"
        " \u2022 Crystal -> cr\n"
        " \u2022 C++ -> cc, cpp, cxx, h, hh, hpp, hxx\n"
        " \u2022 C# -> cs\n"
        " \u2022 D -> d\n"
        " \u2022 Dart -> dart\n"
        " \u2022 Elixir -> ex, exs\n"
        " \u2022 Erlang -> erl, hrl\n"
        " \u2022 Fortran -> f, f90, f95, f03, f08, for\n"
        " \u2022 F# -> fs, fsi, fsx\n"
        " \u2022 Go -> go\n"
        " \u2022 Gradle -> gradle\n"
        " \u2022 Groovy -> groovy\n"
        " \u2022 Haskell -> hs, lhs\n"
        " \u2022 Java -> java\n"
        " \u2022 JavaScript/TypeScript -> cjs, cts, js, jsx, mjs, mts, ts, tsx\n"
        " \u2022 Julia -> jl\n"
        " \u2022 Kotlin -> kt, kts\n"
        " \u2022 Lua -> lua\n"
        " \u2022 Nim -> nim\n"
        " \u2022 Nushell -> nu\n"
        " \u2022 Objective-C -> m, mm\n"
        " \u2022 OCaml -> ml, mli\n"
        " \u2022 Pascal/Delphi -> ldpr, pas, pp\n"
        " \u2022 Perl -> pl, pm, t\n"
        " \u2022 PHP -> php\n"
        " \u2022 PowerShell -> ps1, psm1, psd1\n"
        " \u2022 Python -> py, pyi\n"
        " \u2022 R -> r\n"
        " \u2022 Ruby -> gemspec, rb, rake\n"
        " \u2022 Rust -> rs\n"
        " \u2022 Scala -> sc, scala\n"
        " \u2022 Swift -> swift\n"
        " \u2022 Tcl -> tcl\n"
        " \u2022 V -> v\n"
        " \u2022 Visual Basic -> vb\n"
        " \u2022 Zig -> zig\n"
        "\n",
        stdout);
}

void log_version(void) {
    printf("nvi %s (%s)\n", NVI_VERSION, NVI_BUILD);
    printf("commit %s\n", NVI_COMMIT);
#ifdef __clang__
    printf("clang %d.%d.%d\n", __clang_major__, __clang_minor__, __clang_patchlevel__);
#elif defined(__GNUC__)
    printf("gcc %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif
    printf("%s\n", NVI_TARGET);
    printf("Try 'nvi --help' for more information.\n");
}
