#include "shims.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) && defined(_MSC_VER)
#include <fcntl.h>
#include <io.h>
#include <windows.h>

// present in SDKs >= 10.0.10586; defined here as a fallback for older kits
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

int use_color;
int use_unicode;

void tty_init(void) {
#if defined(_WIN32) && defined(_MSC_VER)
    SetConsoleOutputCP(CP_UTF8);
    // stdout is the machine-readable channel (NUL-delimited pairs or a
    // PowerShell script); text mode would translate '\n' to "\r\n" and corrupt
    // multiline values, so force binary mode. stderr stays in text mode since
    // it only carries human-readable diagnostics.
    _setmode(_fileno(stdout), _O_BINARY);

    // Legacy conhost (the Windows 10 default console) only interprets ANSI
    // escapes when the app opts in via ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    // without it, color codes print literally. Windows Terminal enables it by
    // default, so failure implies a legacy/limited console whose raster fonts
    // often lack the diagnostic glyphs too — degrade both color and unicode
    // together. A redirected stderr also fails GetConsoleMode, which keeps
    // captured log files plain ASCII.
    int vt_enabled = 0;
    HANDLE err_handle = GetStdHandle(STD_ERROR_HANDLE);
    if (err_handle != NULL && err_handle != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(err_handle, &mode)) {
            vt_enabled = SetConsoleMode(err_handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
        }
    }
    use_unicode = vt_enabled;
#else
    use_unicode = 1;
#endif

    // when NO_COLOR is present and not an empty
    // string (regardless of its value), do not add ANSI color
    const char *no_color = getenv("NO_COLOR");
    use_color = isatty(STDERR_FILENO) && (no_color == NULL || no_color[0] == '\0');

#if defined(_WIN32) && defined(_MSC_VER)
    // a console that prints escapes literally is worse than no color at all
    use_color = use_color && vt_enabled;
#endif
}
