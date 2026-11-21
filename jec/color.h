/* JEC dungeon display colors for Rogue 5.2.  Include after <curses.h>. */
#ifndef JROGUE_COLOR_H
#define JROGUE_COLOR_H

#ifndef DEFAULT_COLOR
#define DEFAULT_COLOR 1
#endif
#ifndef DEFAULT_CURSOR
#define DEFAULT_CURSOR 0
#endif

/* This affects presentation only; it is deliberately not part of save state. */
extern bool color_mode;
void jrogue_color_init(void);
void jrogue_color_terminal_restore(void);
void jrogue_color_terminal_resume(void);
void jrogue_color_refresh(void);
/* Use a normal black background for auxiliary curses windows as well. */
void jrogue_color_apply_window(WINDOW *win);
void jrogue_color_normal_attr(void);
int jrogue_color_available(void);
/* Special-purpose art/projectile palettes, independent of map symbols. */
#define JROGUE_PAIR_TOMBSTONE 43
#define JROGUE_PAIR_PROJECTILE_RED 44
#define JROGUE_PAIR_PROJECTILE_CYAN 45
#define JROGUE_PAIR_PROJECTILE_YELLOW 46
#define JROGUE_PAIR_WIN_GREEN 47
#define JROGUE_PAIR_WIN_YELLOW 48
/* Grave art: pale-lavender flowers and muted blue-gray-green ground
 * (palette C).  Dedicated pairs avoid overwriting the A..Z monster colors. */
#define JROGUE_PAIR_GRAVE_FLOWERS 49
#define JROGUE_PAIR_GRAVE_GROUND 50
void jrogue_color_special_attr(int pair);
int jrogue_mvaddch_projectile(int y, int x, chtype ch, int pair);
int jrogue_waddch(WINDOW *win, chtype ch);
int jrogue_mvwaddch(WINDOW *win, int y, int x, chtype ch);
int jrogue_addch(chtype ch);
int jrogue_mvaddch(int y, int x, chtype ch);

#endif /* JROGUE_COLOR_H */
