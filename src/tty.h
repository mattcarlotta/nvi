#ifndef TTY_H
#define TTY_H

extern int use_color;
extern int use_unicode;
void tty_init(void);

// Diagnostic glyphs degrade to ASCII when the console can't render UTF-8
// (legacy Windows conhost; see tty_init). Each macro is a string expression,
// so they compose via %s but cannot be concatenated into string literals.

// •
#define BULLET (use_unicode ? "•" : "*")

// ├
#define TREE_BRANCH (use_unicode ? "├" : "|")

// └
#define TREE_END (use_unicode ? "└" : "`")

// ─
#define TREE_RUNG (use_unicode ? "─" : "-")

// ↠
#define VALUE_ARROW (use_unicode ? "↠" : "->")

#define RED "\x1b[31m"
// #define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define CYAN "\x1b[36m"
#define LIGHT_MAGENTA "\x1b[95m"
#define DARK_GRAY "\x1b[90m"
// #define BOLD_RED "\x1b[1;31m"
#define BOLD_GREEN "\x1b[1;32m"
// #define BOLD_YELLOW "\x1b[1;33m"
#define ITALIC "\x1b[3m"
#define RESET "\x1b[0m"

#endif // TTY_H
