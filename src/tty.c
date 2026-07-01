#include "shims.h"

#if defined(_WIN32) && defined(_MSC_VER)
#include <windows.h>
#endif

int use_color;

void tty_init(void) {
#if defined(_WIN32) && defined(_MSC_VER)
    SetConsoleOutputCP(CP_UTF8);
#endif
    use_color = isatty(STDERR_FILENO);
}
