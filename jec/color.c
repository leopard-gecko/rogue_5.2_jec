/*
 * Presentation-only curses colors for Rogue 5.2.
 *
 * The original rules and map contain ASCII symbols, not colors.  We assign
 * colors only when a symbol is drawn; mimics take the color of their disguise
 * and invisible monsters retain the original visibility rules.
 *
 * The monster RGB values are from rogue52_colors_revised.html, which is a
 * D&D-inspired display palette rather than official D&D color data.
 */
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "rogue.h"
#include "color.h"
#ifdef _WIN32
#include "windows/rogue_windows.h"
#endif

#define MONSTER_FIRST 1
#define NORMAL_PAIR 27
#define WALL_PAIR 28
#define FLOOR_PAIR 29
#define PASSAGE_PAIR 30
#define HERO_PAIR 31
#define TRAP_PAIR 32
#define GOLD_PAIR 33
#define FOOD_PAIR 34
#define WEAPON_PAIR 35
#define ARMOR_PAIR 36
#define POTION_PAIR 37
#define SCROLL_PAIR 38
#define RING_PAIR 39
#define STICK_PAIR 40
#define STAIRS_PAIR 41
#define AMULET_PAIR 42
/* Dedicated effects and art colors corresponding to the Rogue 5.4 palette. */
#define LAST_PAIR JROGUE_PAIR_GRAVE_GROUND

static int initialized;
static int supported;
#ifndef _WIN32
static int terminal_background_black;
#endif
static int cleanup_registered;

/* Match Rogue 5.4.5 JEC: curses only paints cells, whereas OSC 11 also
 * changes the terminal profile background visible in untouched cells.
 * Never use OSC 11 on Windows/PDCurses where raw escapes are unsuitable. */
static void
terminal_colors(int enabled)
{
#ifndef _WIN32
    if (!isatty(STDOUT_FILENO))
        return;
    if (enabled && !terminal_background_black)
    {
        fputs("\033]10;#c0c0c0\007\033]11;#000000\007\033[37m", stdout);
        fputs(COLORS >= 256 ? "\033[48;5;16m" : "\033[40m", stdout);
        fflush(stdout);
        terminal_background_black = TRUE;
    }
    else if (!enabled && terminal_background_black)
    {
        fputs("\033]110\007\033]111\007\033[39;49m", stdout);
        fflush(stdout);
        terminal_background_black = FALSE;
    }
#else
    (void)enabled;
#endif
}

void
jrogue_color_terminal_restore(void)
{
    terminal_colors(FALSE);
}

void
jrogue_color_terminal_resume(void)
{
    if (supported && color_mode)
        terminal_colors(TRUE);
}


/* A..Z, in the same order and with exactly the same HTML RGB values. */
static const unsigned long monster_rgb[26] = {
    0xC76B53UL, 0xB5A7ADUL, 0xE6BEA9UL, 0xD96B53UL,
    0x90C4D5UL, 0xB891DFUL, 0x609CDAUL, 0xC96F63UL,
    0x9AAEC2UL, 0xE8C453UL, 0xBC7A5CUL, 0x85BB69UL,
    0xBAC0C5UL, 0xE1B8CDUL, 0x93AD80UL, 0xA77CDEUL,
    0xC8859AUL, 0xCD8B6DUL, 0x85B99AUL, 0x89AB75UL,
    0x9C9B95UL, 0xC75F68UL, 0xA1B6D0UL, 0xAA9B84UL,
    0xE8EEF5UL, 0x9EAA95UL
};

/* 5.4.5 palette foreground values for non-monster dungeon symbols.
 * Wall and door share 145; floor and passage both use gray 242.
 * All dungeon symbols keep a black background.
 */
static const short symbol_256[AMULET_PAIR - NORMAL_PAIR + 1] = {
    7, 145, 242, 242, 254, 144, 180, 180,
    180, 180, 180, 180, 180, 180, 254, 180
};

/* Standard xterm palette RGB used only for 8/16-color fallback. */
static unsigned long
xterm_rgb(short ix)
{
    static const int levels[6] = {0,95,135,175,215,255};
    int n,r,g,b;
    if (ix >= 232) {n=8+(ix-232)*10;return (unsigned long)(n*65536L+n*256+n);}
    if (ix < 16) return 0xBFBFBFUL;
    ix -= 16;
    r=levels[ix/36];g=levels[(ix/6)%6];b=levels[ix%6];
    return (unsigned long)(r*65536L+g*256+b);
}

static int
square(int x)
{
    return x * x;
}

static short
nearest_xterm(unsigned long rgb)
{
    static const int levels[6] = {0, 95, 135, 175, 215, 255};
    int r = (int)((rgb >> 16) & 255UL);
    int g = (int)((rgb >> 8) & 255UL);
    int b = (int)(rgb & 255UL);
    int i, j, k, gray, dist, best = 0x7fffffff;
    short result = 7;

    for (i = 0; i < 6; ++i)
        for (j = 0; j < 6; ++j)
            for (k = 0; k < 6; ++k)
            {
                dist = square(r-levels[i]) + square(g-levels[j]) + square(b-levels[k]);
                if (dist < best)
                {
                    best = dist;
                    result = (short)(16 + 36*i + 6*j + k);
                }
            }
    for (i = 0; i < 24; ++i)
    {
        gray = 8 + i*10;
        dist = square(r-gray) + square(g-gray) + square(b-gray);
        if (dist < best)
        {
            best = dist;
            result = (short)(232 + i);
        }
    }
    return result;
}

/* Coarse fallback for 8/16-color terminals. */
static short
nearest_basic(unsigned long rgb)
{
    static const int basic[8][3] = {
        {0,0,0}, {205,0,0}, {0,205,0}, {205,205,0},
        {0,0,238}, {205,0,205}, {0,205,205}, {229,229,229}
    };
    int r=(int)((rgb>>16)&255UL),g=(int)((rgb>>8)&255UL),b=(int)(rgb&255UL);
    int i,d,best=0x7fffffff;
    short result=COLOR_WHITE;
    for (i=0;i<8;i++)
    {
        d=square(r-basic[i][0])+square(g-basic[i][1])+square(b-basic[i][2]);
        if (d<best) {best=d;result=(short)i;}
    }
    return result;
}

void
jrogue_color_init(void)
{
    int i;
    short black;
    short fg;

    initialized = TRUE;
    if (!cleanup_registered)
    {
        atexit(jrogue_color_terminal_restore);
        cleanup_registered = TRUE;
    }
    supported = has_colors();
    if (!supported)
        return;
    start_color();
#ifdef _WIN32
    jrogue_windows_init_palette();
#endif
    /* ncurses sets COLORS and COLOR_PAIRS in start_color(), not earlier. */
    supported = COLOR_PAIRS > LAST_PAIR;
    if (!supported)
        return;

    /* Some terminals display palette color 0 as gray.  The xterm 256-color
     * palette defines 16 as true black; keep every map cell on this background. */
    black = COLORS >= 256 ? 16 : COLOR_BLACK;
    for (i = 0; i < 26; i++)
    {
        /* On terminals with a writable palette, use the HTML RGB exactly.
         * Otherwise select the nearest available xterm color. */
        if (COLORS >= 90 && can_change_color())
        {
            unsigned long rgb = monster_rgb[i];
            int rr = (int)((rgb >> 16) & 255UL);
            int gg = (int)((rgb >> 8) & 255UL);
            int bb = (int)(rgb & 255UL);
            short custom = (short)(64 + i);

            /* can_change_color() is a capability hint, not a guarantee
             * that init_color() accepts each requested palette entry.
             * Fall back rather than display an unintended system color. */
            if (init_color(custom, (short)((rr * 1000 + 127) / 255),
                                   (short)((gg * 1000 + 127) / 255),
                                   (short)((bb * 1000 + 127) / 255)) != ERR)
                fg = custom;
            else if (COLORS >= 256)
                fg = nearest_xterm(rgb);
            else
                fg = nearest_basic(rgb);
        }
        else if (COLORS >= 256)
            fg = nearest_xterm(monster_rgb[i]);
        else
            fg = nearest_basic(monster_rgb[i]);
        init_pair((short)(MONSTER_FIRST+i),fg,black);
    }
    for (i = NORMAL_PAIR; i <= AMULET_PAIR; i++)
    {
        if (COLORS >= 256)
            fg=symbol_256[i-NORMAL_PAIR];
        else
            fg=nearest_basic(xterm_rgb(symbol_256[i-NORMAL_PAIR]));
        init_pair((short)i,fg,black);
    }
    /* Tombstone colors: retain the stone (59) and use palette C for
     * the flowers and ground.  Xterm 182 is pale lavender and 109 is a
     * muted, slightly blue-gray green.  Dedicated pairs keep the monster
     * palette untouched.  On 8/16-color terminals use basic approximations. */
    {
        static const short extra_256[] = {
            59, 210, 117, 222, 114, 221, 182, 109
        };
        for (i = JROGUE_PAIR_TOMBSTONE; i <= LAST_PAIR; ++i)
        {
            fg = extra_256[i - JROGUE_PAIR_TOMBSTONE];
            if (COLORS < 256)
            {
                if (i == JROGUE_PAIR_GRAVE_FLOWERS)
                    fg = COLOR_MAGENTA;
                else if (i == JROGUE_PAIR_GRAVE_GROUND)
                    fg = COLOR_GREEN;
                else
                    fg = nearest_basic(xterm_rgb(fg));
            }
            init_pair((short)i, fg, black);
        }
    }
    terminal_colors(color_mode);
    bkgd(' ' | (color_mode ? COLOR_PAIR(NORMAL_PAIR) : A_NORMAL));
    jrogue_color_normal_attr();
}

/* Background and text attributes are kept consistent between stdscr and
 * hw.  A newwin() does not inherit stdscr's bkgd(), so auxiliary screens
 * must be initialized explicitly.  Use only after the window exists. */
void
jrogue_color_apply_window(WINDOW *win)
{
    if (win == NULL)
        return;
    if (initialized && supported && color_mode)
    {
        wbkgd(win, ' ' | COLOR_PAIR(NORMAL_PAIR));
        wattrset(win, COLOR_PAIR(NORMAL_PAIR) | A_NORMAL);
    }
    else
    {
        wbkgd(win, ' ' | A_NORMAL);
        wattrset(win, A_NORMAL);
    }
}

int
jrogue_color_available(void)
{
    return initialized && supported && color_mode;
}

void
jrogue_color_normal_attr(void)
{
    if (jrogue_color_available())
        attrset(COLOR_PAIR(NORMAL_PAIR) | A_NORMAL);
    else
        attrset(A_NORMAL);
}

void
jrogue_color_special_attr(int pair)
{
    if (jrogue_color_available() &&
        pair >= JROGUE_PAIR_TOMBSTONE && pair <= LAST_PAIR)
        attrset(COLOR_PAIR(pair) | A_NORMAL);
    else
        jrogue_color_normal_attr();
}

/* Explicit projectile colors must override punctuation ('/', '-' and '|')
 * which would otherwise be drawn with the wall palette. */
int
jrogue_mvaddch_projectile(int y, int x, chtype ch, int pair)
{
    int result;
    chtype saved;

    if (!jrogue_color_available())
        return mvaddch(y, x, ch);
    saved = getattrs(stdscr);
    attrset((saved & ~A_COLOR) | COLOR_PAIR(pair));
    result = mvaddch(y, x, ch);
    attrset(saved);
    return result;
}

static int
pair_for(int ch)
{
    if (ch >= 'A' && ch <= 'Z') return MONSTER_FIRST + ch-'A';
    switch(ch)
    {
    case '-': case '|': case '+': return WALL_PAIR;
    case '.': return FLOOR_PAIR;
    case '#': return PASSAGE_PAIR;
    case '$': return PASSAGE_PAIR; /* Magic detection, 5.4 foreground 242. */
    case '@': return HERO_PAIR;
    case '^': return TRAP_PAIR;
    case '*': return GOLD_PAIR;
    case ':': return FOOD_PAIR;
    case ')': return WEAPON_PAIR;
    case ']': return ARMOR_PAIR;
    case '!': return POTION_PAIR;
    case '?': return SCROLL_PAIR;
    case '=': return RING_PAIR;
    case '/': return STICK_PAIR;
    case '%': return STAIRS_PAIR;
    case ',': return AMULET_PAIR;
    default: return NORMAL_PAIR;
    }
}

/* Save and restore only the caller's curses attributes.  In particular,
 * don't lose A_STANDOUT, which the game uses for detected monsters. */
int
jrogue_addch(chtype ch)
{
    int result;
    chtype saved;
    if (!jrogue_color_available()) return addch(ch);
    saved = getattrs(stdscr);
    attrset((saved & ~A_COLOR) | COLOR_PAIR(pair_for((int)(ch & A_CHARTEXT))));
    result = addch(ch);
    attrset(saved);
    return result;
}

int
jrogue_mvaddch(int y, int x, chtype ch)
{
    int result;
    chtype saved;
    if (!jrogue_color_available()) return mvaddch(y,x,ch);
    saved = getattrs(stdscr);
    attrset((saved & ~A_COLOR) | COLOR_PAIR(pair_for((int)(ch & A_CHARTEXT))));
    result = mvaddch(y,x,ch);
    attrset(saved);
    return result;
}

/* The curses window used for inventory, help, item/gold detection and
 * the wizard map needs the same glyph-to-palette mapping as stdscr. */
int
jrogue_waddch(WINDOW *win, chtype ch)
{
    int result;
    chtype saved;

    if (!jrogue_color_available())
        return waddch(win, ch);
    saved = getattrs(win);
    wattrset(win, (saved & ~A_COLOR) |
             COLOR_PAIR(pair_for((int)(ch & A_CHARTEXT))));
    result = waddch(win, ch);
    wattrset(win, saved);
    return result;
}

int
jrogue_mvwaddch(WINDOW *win, int y, int x, chtype ch)
{
    int result;
    chtype saved;

    if (!jrogue_color_available())
        return mvwaddch(win, y, x, ch);
    saved = getattrs(win);
    wattrset(win, (saved & ~A_COLOR) |
             COLOR_PAIR(pair_for((int)(ch & A_CHARTEXT))));
    result = mvwaddch(win, y, x, ch);
    wattrset(win, saved);
    return result;
}

/* Repaint only the existing visible ASCII dungeon map, not the current
 * message (which may contain multibyte Japanese) or the cached status.
 * A color change must not rerun visibility, consume turns, or expose hidden
 * monsters.  Snapshot before bkgd() modifies stored curses attributes. */
void
jrogue_color_refresh(void)
{
    int y, x, oldy, oldx, rows, cols;
    chtype *map;
    chtype cell;
    size_t index;

    if (!initialized || !supported || stdscr == NULL)
        return;

    rows = LINES;
    cols = COLS;
    getyx(stdscr, oldy, oldx);
    if (rows <= 2 || cols <= 0)
        return;

    map = (chtype *)malloc((size_t)(rows - 2) * (size_t)cols * sizeof *map);
    if (map == NULL)
    {
        /* Keep the terminal background in sync even if allocation fails.
         * Never redraw from live curses cells after changing bkgd(). */
        terminal_colors(color_mode);
        bkgd(' ' | (color_mode ? COLOR_PAIR(NORMAL_PAIR) : A_NORMAL));
        jrogue_color_normal_attr();
        jrogue_status_redraw();
        clearok(stdscr, TRUE);
        return;
    }

    for (y = 1; y < rows - 1; ++y)
        for (x = 0; x < cols; ++x)
            map[(size_t)(y - 1) * (size_t)cols + (size_t)x] = mvinch(y, x);

    terminal_colors(color_mode);
    bkgd(' ' | (color_mode ? COLOR_PAIR(NORMAL_PAIR) : A_NORMAL));
    jrogue_color_normal_attr();

    /* chgat() changes cell attributes without replacing the UTF-8 text.
     * Rewriting mvinch() character codes on this line corrupts Japanese. */
    mvchgat(0, 0, cols, A_NORMAL,
            (short)(color_mode ? NORMAL_PAIR : 0), NULL);

    for (y = 1; y < rows - 1; ++y)
        for (x = 0; x < cols; ++x)
        {
            index = (size_t)(y - 1) * (size_t)cols + (size_t)x;
            cell = map[index];
            if (cell == (chtype)ERR)
                continue;
            cell = (cell & A_CHARTEXT) |
                   (cell & (A_STANDOUT | A_REVERSE));
            /* Explicitly clear the old pair before monochrome painting. */
            if (!color_mode)
                attrset(A_NORMAL);
            jrogue_mvaddch(y, x, cell);
        }

    free(map);
    /* status() caches its numbers, so a plain call may print nothing. */
    jrogue_status_redraw();
    jrogue_color_normal_attr();
    move(oldy, oldx);
    touchwin(stdscr);
    clearok(stdscr, TRUE);
}
