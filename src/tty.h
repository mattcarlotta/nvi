#ifndef TTY_H
#define TTY_H

extern int use_color;
void tty_init(void);

// \u250C
// #define TREE_BEGIN "┌"

// \u2502
// #define TREE_STEM "│"

// \u251C
#define TREE_BRANCH "├"

// \u2514
#define TREE_END "└"

#define RED "\x1b[31m"
// #define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define CYAN "\x1b[36m"
#define DARK_GRAY "\x1b[90m"
// #define BOLD_RED "\x1b[1;31m"
#define BOLD_GREEN "\x1b[1;32m"
#define BOLD_YELLOW "\x1b[1;33m"
#define RESET "\x1b[0m"

#endif
