#include "shims.h"

#ifdef _WIN32
#include <windows.h>
#endif

int use_color;

void tty_init(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    use_color = isatty(STDERR_FILENO);
}
