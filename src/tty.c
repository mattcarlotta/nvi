#include "shims.h"

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
    use_color = isatty(STDERR_FILENO);
}
