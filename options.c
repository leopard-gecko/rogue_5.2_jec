/*
 * This file has all the code for the option command.  I would rather
 * this command were not necessary, but it is the only way to keep the
 * wolves off of my back.
 *
 * @(#)options.c	4.12 (Berkeley) 3/2/82
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "rogue.h"
#include "jec/unicode_text.h"
#include "jec/color.h"
#ifdef _WIN32
#include "windows/rogue_windows.h"
#endif

#define	EQSTR(a, b, c)	(strncmp(a, b, c) == 0)

#define	NUM_OPTS	(sizeof optlist / sizeof (OPTION))
#define OPT_REDRAW	3

/*
 * description of an option and what to do with it
 */
struct optstruct {
    char	*o_name;	/* option name */
    char	*o_prompt;	/* prompt for interactive entry */
    void		*o_opt;		/* pointer to thing to set */
    void	(*o_putfunc)(void *);	/* function to print value */
    int		(*o_getfunc)(void *, WINDOW *);	/* function to get value interactively */
};

typedef struct optstruct	OPTION;

static int get_option_char(WINDOW *win);
#ifdef _WIN32
/* When the Windows console API is unavailable (e.g. redirected or MSYS
 * pseudo terminal input), wgetch() can still return UTF-8 byte sequences.
 * Distinguish those bytes from Unicode scalar values returned by Win32. */
static int option_key_is_unicode;
#endif
static int utf8_read_sequence(WINDOW *win, int first, char *out, int *outlen);
static void redraw_input(WINDOW *win, int oy, int ox, const char *buf, int pos);
static int insert_bytes(char *buf, int *lenp, int *posp, const char *bytes, int nbytes, int maxinp);
static void delete_range(char *buf, int *lenp, int start, int end);
static void pr_optname(OPTION *op);
static void option_redraw(void);
static void put_lang(void *vp);
static int get_lang(void *vp, WINDOW *win);
static int get_str_common(void *optp, WINDOW *win, int force_size);
static int option_bufsize(char *opt);
static void strucpy_limit(char *s1, char *s2, int len, int dstsize);

/* Keep emoji as complete Unicode scalars in PDCursesMod's wide cells.
 * A plain waddstr() may send a four-byte UTF-8 sequence as four glyphs. */
static int
option_waddstr(WINDOW *win, const char *s)
{
#ifdef _WIN32
    return jrogue_windows_waddstr_utf8(win, s);
#else
    return waddstr(win, s);
#endif
}

static void
option_refresh(WINDOW *win)
{
    wrefresh(win);
#ifdef _WIN32
    jrogue_windows_option_vt_restore_nonbmp(win);
#endif
}

OPTION	optlist[] = {
    {"terse",   (char *) JSTR_STATIC("簡潔な表示", "Terse output"),
		 &terse,        put_bool, get_bool },
    {"flush",   (char *) JSTR_STATIC("戦闘時に先行入力をキャンセル", "Flush typeahead during battle"),
		&fight_flush,  put_bool, get_bool },
    {"jump",    (char *) JSTR_STATIC("移動中の表示を行わない", "Show position only at end of run"),
		&jump,         put_bool, get_bool },
    {"step",    (char *) JSTR_STATIC("持ちものを１行ずつ表示", "Do inventories one line at a time"),
		&slow_invent,  put_bool, get_bool },
    {"askme",   (char *) JSTR_STATIC("未識別の品物について尋ねる", "Ask me about unidentified things"),
		&askme,        put_bool, get_bool },
    {"passgo",  (char *) JSTR_STATIC("通路の角で止まらずに進む", "Follow turnings in passageways"),
		&passgo,       put_bool, get_bool },
    {"color",   (char *) JSTR_STATIC("カラー表示", "Color display"),
		&color_mode,   put_bool, get_bool },
    {"cursor",  (char *) JSTR_STATIC("プレイ中のカーソルを表示", "Show cursor during play"),
		&show_cursor,  put_bool, get_bool },
    {"name",    (char *) JSTR_STATIC("プレイヤー名", "Name"),
		whoami,        put_str,  get_str  },
    {"fruit",   (char *) JSTR_STATIC("好きな果物", "Fruit"),
		fruit,         put_str,  get_str  },
    {"language", (char *) JSTR_STATIC("表示する言語", "Language"),
		&jrogue_lang_is_japanese, put_lang, get_lang },
    {"file",    (char *) JSTR_STATIC("セーブファイル名", "Save file"),
		file_name,     put_str,  get_str  }
};


void
jrogue_apply_options_language(void)
{
    optlist[0].o_prompt = (char *) JSTR("簡潔な表示", "Terse output");
    optlist[1].o_prompt = (char *) JSTR("戦闘時に先行入力をキャンセル", "Flush typeahead during battle");
    optlist[2].o_prompt = (char *) JSTR("移動中の表示を行わない", "Show position only at end of run");
    optlist[3].o_prompt = (char *) JSTR("持ちものを１行ずつ表示", "Do inventories one line at a time");
    optlist[4].o_prompt = (char *) JSTR("未識別の品物について尋ねる", "Ask me about unidentified things");
    optlist[5].o_prompt = (char *) JSTR("通路の角で止まらずに進む", "Follow turnings in passageways");
    optlist[6].o_prompt = (char *) JSTR("カラー表示", "Color display");
    optlist[7].o_prompt = (char *) JSTR("プレイ中のカーソルを表示", "Show cursor during play");
    optlist[8].o_prompt = (char *) JSTR("プレイヤー名", "Name");
    optlist[9].o_prompt = (char *) JSTR("好きな果物", "Fruit");
    optlist[10].o_prompt = (char *) JSTR("表示する言語", "Language");
    optlist[11].o_prompt = (char *) JSTR("セーブファイル名", "Save file");
}

static int
option_bufsize(char *opt)
{
    if (opt == file_name || opt == home)
	return PATHSTR;
    if (opt == prbuf || opt == huh)
	return PRBUF_SIZE;
    return MAXSTR;
}

static void
pr_optname(OPTION *op)
{
    wprintw(hw, "%s (\"%s\"): ", op->o_prompt, op->o_name);
}

static void
option_redraw(void)
{
    OPTION *op;

    jrogue_color_apply_window(hw);
    wclear(hw);
    clearok(hw, TRUE);
    touchwin(hw);
    for (op = optlist; op < &optlist[NUM_OPTS]; op++)
    {
	pr_optname(op);
	(*op->o_putfunc)(op->o_opt);
	waddch(hw, '\n');
    }
    option_refresh(hw);
}

/*
 * option:
 *	Print and then set options from the terminal
 */
void option(void)
{
    register OPTION	*op;
    register int	retval;

    option_redraw();

    /* The option editor always shows a visible input cursor, even when
     * gameplay is configured to hide it. */
    jrogue_show_cursor(TRUE);

    /*
     * Set values
     */
    wmove(hw, 0, 0);
    for (op = optlist; op < &optlist[NUM_OPTS]; op++)
    {
	pr_optname(op);
        jrogue_show_cursor(TRUE);
	if ((retval = (*op->o_getfunc)(op->o_opt, hw)))
	{
	    if (retval == QUIT)
		break;
	    else if (retval == OPT_REDRAW)
	    {
		option_redraw();
		wmove(hw, (op - optlist) + 1, 0);
	    }
	    else if (op > optlist) {	/* MINUS */
		wmove(hw, (op - optlist) - 1, 0);
		op -= 2;
	    }
	    else	/* trying to back up beyond the top */
	    {
		putchar('');
		wmove(hw, 0, 0);
		op--;
	    }
	}
    }
    /*
     * Switch back to original screen
     */
    wmove(hw, LINES - 1, 0);
    option_waddstr(hw, JSTR("--スペースを押してください--", "--Press space to continue--"));
    option_refresh(hw);
    jrogue_show_cursor(TRUE);
    w_wait_for(hw, ' ');
    clearok(curscr, TRUE);
    touchwin(stdscr);
    jrogue_color_refresh();
    jrogue_restore_cursor();
    after = FALSE;
}

/*
 * put_bool
 *	Put out a boolean
 */
void put_bool(void *opt)
{
    bool *b = (bool *)opt;
    waddstr(hw, *b ? JSTR("有効(T)", "True") : JSTR("無効(F)", "False"));
}

/*
 * put_str:
 *	Put out a string
 */
void put_str(void *opt)
{
    char *str = (char *)opt;
    option_waddstr(hw, str);
}

static void
put_lang(void *vp)
{
    int *jp = (int *) vp;

    if (*jp)
	waddstr(hw, JSTR("日本語(J)", "Japanese(J)"));
    else
	waddstr(hw, JSTR("英語(E)", "English(E)"));
}

static int
get_lang(void *vp, WINDOW *win)
{
    int *jp = (int *) vp;
    int old_lang;
    int oy, ox;

    old_lang = *jp;
    getyx(win, oy, ox);
    put_lang(vp);

    for (;;)
    {
	wmove(win, oy, ox);
	option_refresh(win);
	switch (readcharw(win))
	{
	    case 'j':
	    case 'J':
		*jp = TRUE;
		break;
	    case 'e':
	    case 'E':
		*jp = FALSE;
		break;
	    case '\n':
	    case '\r':
		wmove(win, oy, ox);
		put_lang(vp);
		wclrtoeol(win);
		waddch(win, '\n');
		return NORM;
	    case ESCAPE:
	    case CTRL('G'):
		return QUIT;
	    case '-':
		return MINUS;
	    default:
		mvwaddstr(win, oy, ox + 15, JSTR("［日本語(J)、英語(E)］", "(J or E)"));
		continue;
	}
	break;
    }

    jrogue_apply_language();
    wmove(win, oy, ox);
    put_lang(vp);
    wclrtoeol(win);
    waddch(win, '\n');

    return (*jp != old_lang) ? OPT_REDRAW : NORM;
}

/*
 * get_bool:
 *	Allow changing a boolean option and print it out
 */
int get_bool(void *opt, WINDOW * win)
{
    bool *bp = (bool *)opt;
    register int oy, ox;
    register bool op_bad;

    op_bad = TRUE;
    getyx(win, oy, ox);
    waddstr(win, *bp ? JSTR("有効(T)", "True") : JSTR("無効(F)", "False"));
    while (op_bad)	
    {
	wmove(win, oy, ox);
	option_refresh(win);
	switch (readcharw(win))
	{
	    case 't':
	    case 'T':
		*bp = TRUE;
		op_bad = FALSE;
		break;
	    case 'f':
	    case 'F':
		*bp = FALSE;
		op_bad = FALSE;
		break;
	    case '\n':
	    case '\r':
		op_bad = FALSE;
		break;
	    case '\033':
	    case '\007':
		return QUIT;
	    case '-':
		return MINUS;
	    default:
		mvwaddstr(win, oy, ox + 10, JSTR("［有効(T)、無効(F)］", "(T or F)"));
	}
    }
    wmove(win, oy, ox);
    waddstr(win, *bp ? JSTR("有効(T)", "True") : JSTR("無効(F)", "False"));
    waddch(win, '\n');
    return NORM;
}

/*
 * get_str:
 *	Set a string option
 */
#define MAXINP	(MAXSTR - 1)	/* default max bytes for fixed legacy strings */

static int
get_str_common(void *optp, WINDOW * win, int force_size)
{
    char *opt = (char *)optp;
    int c, oy, ox;
    int len, pos, bufsiz, maxinp;
    char buf[PATHSTR];
    char bytes[8];
    int nbytes;

    getyx(win, oy, ox);
    bufsiz = force_size > 0 ? force_size : option_bufsize(opt);
    if (bufsiz > (int) sizeof buf)
	bufsiz = (int) sizeof buf;
    maxinp = bufsiz - 1;

    /*
     * The original Rogue input field was byte-oriented and started with an
     * empty temporary buffer.  For UTF-8 names and fruit, keep the current
     * value in the edit buffer and edit by character boundaries.
     */
    utf8_strlcpy(buf, opt, bufsiz);
    len = strlen(buf);
    pos = len;

    keypad(win, TRUE);
    meta(win, TRUE);
    if (win == stdscr)
        jrogue_color_normal_attr();
    else
        jrogue_color_apply_window(win);
    jrogue_show_cursor(TRUE);
    redraw_input(win, oy, ox, buf, pos);

    for (;;)
    {
        c = get_option_char(win);

        if (c == -1)
            continue;
        if (c == '\n' || c == '\r')
            break;
        if (c == '\033' || c == '\007')
        {
            redraw_input(win, oy, ox, opt, (int)strlen(opt));
            if (win != stdscr)
            {
                waddch(win, '\n');
                option_refresh(win);
            }
            jrogue_restore_cursor();
            return QUIT;
        }
        if (c == '-' && len == 0 && win != stdscr)
        {
            jrogue_restore_cursor();
            return MINUS;
        }

        if (c == md_erasechar() || c == '\b' || c == 0177 || c == KEY_BACKSPACE)
        {
            if (pos > 0)
            {
                int prev;

                prev = jrogue_utf8_prev_index(buf, pos);
                delete_range(buf, &len, prev, pos);
                pos = prev;
                redraw_input(win, oy, ox, buf, pos);
            }
            else
                putchar(CTRL('G'));
            continue;
        }

        if (c == md_killchar() || c == CTRL('U'))
        {
            buf[0] = '\0';
            len = 0;
            pos = 0;
            redraw_input(win, oy, ox, buf, pos);
            continue;
        }

        if (c == KEY_LEFT)
        {
            if (pos > 0)
                pos = jrogue_utf8_prev_index(buf, pos);
            else
                putchar(CTRL('G'));
            redraw_input(win, oy, ox, buf, pos);
            continue;
        }

        if (c == KEY_RIGHT)
        {
            if (pos < len)
                pos = jrogue_utf8_next_index(buf, len, pos);
            else
                putchar(CTRL('G'));
            redraw_input(win, oy, ox, buf, pos);
            continue;
        }

#ifdef KEY_HOME
        if (c == KEY_HOME)
        {
            pos = 0;
            redraw_input(win, oy, ox, buf, pos);
            continue;
        }
#endif
#ifdef KEY_END
        if (c == KEY_END)
        {
            pos = len;
            redraw_input(win, oy, ox, buf, pos);
            continue;
        }
#endif

#ifdef KEY_DC
        if (c == KEY_DC)
        {
            if (pos < len)
            {
                int next;

                next = jrogue_utf8_next_index(buf, len, pos);
                delete_range(buf, &len, pos, next);
                redraw_input(win, oy, ox, buf, pos);
            }
            else
                putchar(CTRL('G'));
            continue;
        }
#endif

        if (len == 0 && c == '~')
        {
            utf8_strlcpy(buf, home, sizeof buf);
            len = strlen(buf);
            pos = len;
            redraw_input(win, oy, ox, buf, pos);
            continue;
        }

        if (!utf8_read_sequence(win, c, bytes, &nbytes))
        {
            putchar(CTRL('G'));
            continue;
        }

        if (!insert_bytes(buf, &len, &pos, bytes, nbytes, maxinp))
            putchar(CTRL('G'));
        redraw_input(win, oy, ox, buf, pos);
    }

    utf8_trim_partial(buf);
    strucpy_limit(opt, buf, strlen(buf), bufsiz);
    redraw_input(win, oy, ox, opt, (int)strlen(opt));
    if (win != stdscr)
    {
        waddch(win, '\n');
        option_refresh(win);
    }
    if (win == stdscr)
        mpos += jrogue_utf8_display_width_n(buf, strlen(buf));
    jrogue_restore_cursor();
    return NORM;
}

int
get_str(void *optp, WINDOW *win)
{
    return get_str_common(optp, win, 0);
}

int
get_str_size(void *optp, WINDOW *win, int bufsiz)
{
    return get_str_common(optp, win, bufsiz);
}

#ifdef WIZARD
/*
 * get_num:
 *	Get a numeric option
 */
int get_num(short * opt, WINDOW * win)
{
    register int i;
    char buf[MAXSTR];

    buf[0] = '\0';
    if ((i = get_str(buf, win)) == NORM)
	*opt = atoi(buf);
    return i;
}
#endif

/*
 * parse_opts:
 *	Parse options from string, usually taken from the environment.
 *	The string is a series of comma seperated values, with booleans
 *	being stated as "name" (true) or "noname" (false), and strings
 *	being "name=....", with the string being defined up to a comma
 *	or the end of the entire option string.
 */
void parse_opts(char * str)
{
    register char *sp;
    register OPTION *op;
    register int len;

    while (*str)
    {
	/*
	 * Get option name
	 */
	for (sp = str; isalpha((unsigned char)*sp); sp++)
	    continue;
	len = sp - str;
	/*
	 * Look it up and deal with it
	 */
	for (op = optlist; op < &optlist[NUM_OPTS]; op++)
	    if (EQSTR(str, op->o_name, len))
	    {
		if (op->o_putfunc == put_bool)	/* if option is a boolean */
		    *(bool *)op->o_opt = TRUE;
		else
		{
		    register char *start;
		    /*
		     * Skip to start of string value
		     */
		    for (str = sp + 1; *str == '='; str++)
			continue;
		    /*
		     * Skip to end of string value
		     */
		    for (sp = str + 1; *sp && *sp != ','; sp++)
			continue;
		    if (op->o_getfunc == get_lang)
		    {
			char langbuf[MAXSTR];
			strucpy_limit(langbuf, str, sp - str, sizeof langbuf);
			if (jrogue_set_language(langbuf))
			    jrogue_apply_language();
		    }
		    else
		    {
			if (*str == '~')
			{
			    char *base = (char *) op->o_opt;
			    int osize = option_bufsize(base);

			    strucpy_limit(base, home, strlen(home), osize);
			    start = base + strlen(base);
			    while (*++str == '/')
				continue;
			    if (*str != '\0' && start > base && start[-1] != '/'
			        && (int)(start - base) < osize - 1)
			    {
				*start++ = '/';
				*start = '\0';
			    }
			}
			else
			    start = (char *) op->o_opt;
			strucpy_limit(start, str, sp - str, option_bufsize((char *) op->o_opt) - (int)(start - (char *) op->o_opt));
		    }
		}
		break;
	    }
	    /*
	     * check for "noname" for booleans
	     */
	    else if (op->o_putfunc == put_bool
	      && EQSTR(str, "no", 2) && EQSTR(str + 2, op->o_name, len - 2))
	    {
		*(bool *)op->o_opt = FALSE;
		break;
	    }

	/*
	 * skip to start of next option name
	 */
	while (*sp && !isalpha((unsigned char)*sp))
	    sp++;
	str = sp;
    }
}

static int
get_option_char(WINDOW *win)
{
    int c;

#ifdef _WIN32
    option_key_is_unicode = jrogue_windows_read_option_key(&c);
    if (option_key_is_unicode)
        return c;
#endif
    c = wgetch(win);
    return c;
}

static int
utf8_read_sequence(WINDOW *win, int first, char *out, int *outlen)
{
    int need, i, c;
    unsigned char ch;

#ifdef _WIN32
    /* ReadConsoleInputW yields full Unicode scalars, not UTF-8 bytes. */
    if (option_key_is_unicode && first >= 0x80 && first <= 0x10ffff)
    {
        *outlen = jrogue_utf8_encode((unsigned long)first, out);
        return *outlen > 0;
    }
#endif
    if (first < 0 || first > 255)
        return FALSE;

    ch = (unsigned char) first;
    need = jrogue_utf8_expected_len(ch);
    if (need <= 0)
        return FALSE;

    if (need == 1)
    {
        if (!jrogue_utf8_copyable_byte(ch))
            return FALSE;
        out[0] = (char) ch;
        *outlen = 1;
        return TRUE;
    }

    out[0] = (char) ch;
    for (i = 1; i < need; i++)
    {
        c = get_option_char(win);
        if (c < 0 || c > 255 || !jrogue_utf8_continuation((unsigned char)c))
            return FALSE;
        out[i] = (char)c;
    }
    out[need] = '\0';
    *outlen = need;
    return TRUE;
}

/*
 * Edit a full path without allowing it to wrap over dungeon row 1.
 * This only scrolls the visible slice; buf retains the complete pathname.
 */
static void
redraw_input(WINDOW *win, int oy, int ox, const char *buf, int pos)
{
    int avail, len, start, col, leading, wincols;
    char shown[PATHSTR];

    wincols = getmaxx(win);
    if (wincols > COLS)
        wincols = COLS;
    avail = wincols - ox - 1;  /* spare the final column */
    if (avail <= 0)
        return;
    len = strlen(buf);
    if (pos < 0)
        pos = 0;
    if (pos > len)
        pos = len;
    start = 0;
    while (start < pos &&
           jrogue_utf8_display_width_n(buf + start, pos - start) >
               avail - (start > 0 ? 1 : 0))
        start = jrogue_utf8_next_index(buf, len, start);
    leading = start > 0 ? 1 : 0;

    utf8_display_truncate(shown, buf + start, avail - leading,
                          sizeof shown);
    wmove(win, oy, ox);
    if (leading)
        waddch(win, '<');         /* input continues to the left */
    option_waddstr(win, shown);
    wclrtoeol(win);

    col = ox + leading + jrogue_utf8_display_width_n(buf + start, pos - start);
    if (col >= wincols)
        col = wincols - 1;
    wmove(win, oy, col);
    option_refresh(win);
}

static int
insert_bytes(char *buf, int *lenp, int *posp, const char *bytes, int nbytes, int maxinp)
{
    int len, pos;

    len = *lenp;
    pos = *posp;
    if (nbytes <= 0)
        return FALSE;
    if (len + nbytes >= maxinp)
        return FALSE;

    memmove(buf + pos + nbytes, buf + pos, (size_t)(len - pos + 1));
    memcpy(buf + pos, bytes, (size_t)nbytes);
    *lenp = len + nbytes;
    *posp = pos + nbytes;
    utf8_trim_partial(buf);
    *lenp = strlen(buf);
    if (*posp > *lenp)
        *posp = *lenp;
    return TRUE;
}

static void
delete_range(char *buf, int *lenp, int start, int end)
{
    int len;

    len = *lenp;
    if (start < 0)
        start = 0;
    if (end > len)
        end = len;
    if (start >= end)
        return;
    memmove(buf + start, buf + end, (size_t)(len - end + 1));
    *lenp = len - (end - start);
}

/*
 * strucpy:
 *	Copy option strings, preserving UTF-8 bytes while rejecting controls.
 */
static void
strucpy_limit(char *s1, char *s2, int len, int dstsize)
{
    int copied, maxcopy;

    if (dstsize <= 0)
	return;
    maxcopy = dstsize - 1;
    if (len > maxcopy)
	len = maxcopy;

    copied = 0;
    while (len-- && copied < maxcopy)
    {
        unsigned char c = (unsigned char) *s2++;

	if (jrogue_utf8_copyable_byte(c))
	    s1[copied++] = (char) c;
    }
    s1[copied] = '\0';
    utf8_trim_partial(s1);
}

void strucpy(char * s1, char * s2, int len)
{
    strucpy_limit(s1, s2, len, MAXSTR);
}
