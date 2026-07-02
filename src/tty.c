#include "shims.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) && defined(_MSC_VER)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

int use_color;

void tty_init(void) {
#if defined(_WIN32) && defined(_MSC_VER)
    SetConsoleOutputCP(CP_UTF8);
    // stdout is the machine-readable channel (NUL-delimited pairs or a
    // PowerShell script); text mode would translate '\n' to "\r\n" and corrupt
    // multiline values, so force binary mode. stderr stays in text mode since
    // it only carries human-readable diagnostics.
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    // https://no-color.org/ : when NO_COLOR is present and not an empty
    // string (regardless of its value), do not add ANSI color
    const char *no_color = getenv("NO_COLOR");
    use_color = isatty(STDERR_FILENO) && (no_color == NULL || no_color[0] == '\0');
}
