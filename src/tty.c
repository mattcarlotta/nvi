#include <unistd.h>

int use_color;

void tty_init(void) { use_color = isatty(STDERR_FILENO); }
