/*
 * Various input/output functions
 *
 * @(#)io.c	4.13 (Berkeley) 2/25/82
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
#include "rogue.h"
#include "jec/color.h"
#include "jec/unicode_text.h"
#include <stdarg.h>
#ifdef _WIN32
#include "windows/rogue_windows.h"
#elif !defined(__DJGPP__)
#include <unistd.h>
#endif

/* Finish curses before resetting the cursor shape: PDCursesMod's
 * endwin() selects a blinking block after the normal visibility
 * changes.  Portable builds still call plain endwin(). */
int rogue_endwin(void)
{
    int result = endwin();
#ifdef _WIN32
    if (result != ERR)
    {
        jrogue_windows_cursor_invalidate();
        jrogue_windows_restore_cursor_shape();
    }
#endif
    return result;
}

/*
 * Clear the plain terminal after curses has returned control to it.
 * In a macOS Terminal alternate-screen session, endwin() can expose
 * the startup greeting again.  Use this for scores and successful
 * save-and-exit, but not for the explicit -s score listing.
 * Never send terminal escape sequences to a redirected output stream.
 * Windows uses its own console helper so both VT and classic console
 * output are handled without changing the macOS/Linux implementation.
 */
void
rogue_clear_plain_screen(void)
{
#ifdef _WIN32
    jrogue_windows_clear_plain_screen();
#elif !defined(__DJGPP__)
    if (isatty(STDOUT_FILENO))
    {
        fputs("\033[0m\033[H\033[2J", stdout);
        fflush(stdout);
    }
#endif
}


/*
 * msg:
 *	Display a message at the top of the screen.
 */
static char msgbuf[BUFSIZ];
static int newpos = 0;

void msg(const char *fmt, ...)
{
    va_list ap;
    /*
     * if the string is "", just clear the line
     */
    if (*fmt == '\0')
    {
	move(0, 0);
	clrtoeol();
	mpos = 0;
	newpos = 0;
	msgbuf[0] = '\0';
	return;
    }
    /*
     * otherwise add to the message and flush it out
     */
    va_start(ap,fmt);
    doadd(fmt, ap);
    va_end(ap);
    endmsg();
}

/*
 * addmsg:
 *	Add things to the current message
 */

void addmsg(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    doadd(fmt, ap);
    va_end(ap);
}

/*
 * endmsg:
 *	Display a new msg (giving him a chance to see the previous one
 *	if it is up there with the --More--)
 */

static int
more_col(void)
{
    int prompt_width, maxcol;

    prompt_width = rogue_display_width(JSTR(" --続く--", " --More--"));
    maxcol = COLS - prompt_width;
#ifdef _WIN32
    if (maxcol > 0)
        --maxcol; /* Leave the final terminal cell unused. */
#endif
    if (maxcol < 0)
	maxcol = 0;
    if (mpos > maxcol)
	return maxcol;
    return mpos;
}

void endmsg(void)
{
    if (save_msg)
	utf8_strlcpy(huh, msgbuf, MSGHIST_SIZE);

    if (mpos)
    {
	look(FALSE);
	move(0, more_col());
	addstr(JSTR(" --続く--", " --More--"));
	refresh();
	wait_for(' ');
    }
    /*
     * English messages should start with uppercase, except ones that
     * start with a pack addressing character.  Do not apply ctype/toupper
     * to Japanese UTF-8 text: changing a single byte of a multibyte
     * character corrupts the beginning of the message.
     */
    if (!jrogue_lang_is_japanese
	&& islower((unsigned char)msgbuf[0]) && msgbuf[1] != ')')
	msgbuf[0] = toupper((unsigned char)msgbuf[0]);
    {
	char dispbuf[BUFSIZ];
	/* Reserve the last cell: output there can wrap into dungeon row 1. */
	utf8_display_truncate(dispbuf, msgbuf, COLS > 1 ? COLS - 1 : 0,
	                      sizeof dispbuf);
	mvaddstr(0, 0, dispbuf);
	clrtoeol();
	mpos = rogue_display_width(dispbuf);
    }
    newpos = 0;
    refresh();
}

/*
 * doadd:
 *	Perform an add onto the message buffer
 */

void doadd(const char *fmt, va_list ap)
{
    int avail;

    if (newpos < 0)
        newpos = 0;
    if (newpos >= (int) sizeof msgbuf)
        newpos = (int) sizeof msgbuf - 1;

    avail = (int) sizeof msgbuf - newpos;
    if (avail > 0)
    {
        vsnprintf(&msgbuf[newpos], avail, fmt, ap);
        msgbuf[sizeof msgbuf - 1] = '\0';
        utf8_trim_partial(msgbuf);
    }
    newpos = strlen(msgbuf);
}

/*
 * step_ok:
 *	Returns true if it is ok to step on ch
 */
int step_ok(int ch)
{
    switch (ch)
    {
	case ' ':
	case '|':
	case '-':
	    return FALSE;
	default:
	    return (!isalpha((unsigned char)ch));
    }
}

/*
 * readchar:
 *	Flushes stdout so that screen is up to date and then returns
 *	getchar().
 */
int readcharw(WINDOW * win)
{
    int ch;

    ch = md_readchar(win);

    if ((ch == 3) || (ch == 0))
    {
	quit(0);
	return(27);
    }

    return(ch);
}

int readchar(void)
{
    return( readcharw(stdscr) );
}

char * unctrol(char ch)
{
    return( (char *) unctrl(ch) );
}

/*
 * status:
 *	Display the important stats line.  Keep the cursor where it was.
 */
static int status_force_redraw;

void
jrogue_status_redraw(void)
{
    status_force_redraw = TRUE;
    status();
}

void status(void)
{
    register int oy, ox, temp;
    static int hpwidth = 0, s_hungry;
    static int s_lvl, s_pur = -1, s_hp, s_ac = 0;
    static int s_lang_is_japanese = -1;
    static str_t s_str;
    static long s_exp = 0;
    const char *state_name[] =
    {
	"", JSTR("空腹", "Hungry"), JSTR("飢餓", "Weak"), JSTR("瀕死", "Faint")
    };

    /*
     * If nothing has changed since the last status, don't
     * bother.
     */
    if (!status_force_redraw && s_hp == pstats.s_hpt &&
        s_exp == pstats.s_exp && s_pur == purse
	&& s_ac == (cur_armor != NULL ? cur_armor->o_ac : pstats.s_arm)
	&& s_str == pstats.s_str && s_lvl == level && s_hungry == hungry_state
	&& s_lang_is_japanese == jrogue_lang_is_japanese)
	    return;
	
    status_force_redraw = FALSE;
    getyx(stdscr, oy, ox);
    if (s_hp != max_hp)
    {
	temp = s_hp = max_hp;
	for (hpwidth = 0; temp; hpwidth++)
	    temp /= 10;
    }
    move(LINES - 1, 0);
    /* Repainting also removes old color/standout from the entire line. */
    jrogue_color_normal_attr();
    clrtoeol();
    printw(JSTR("階: %d  金塊: %-5d  体力: %*d(%*d)  強さ: %2d(%d)  守備: %-2d  経験: %d/%ld  %s",
	       "Level: %d  Gold: %-5d  Hp: %*d(%*d)  Str: %2d(%d)  Ac: %-2d  Exp: %d/%ld  %s"),
	    level, purse, hpwidth, pstats.s_hpt, hpwidth, max_hp, pstats.s_str,
	    max_stats.s_str,
	    cur_armor != NULL ? cur_armor->o_ac : pstats.s_arm, pstats.s_lvl,
	    pstats.s_exp, state_name[hungry_state]);

    clrtoeol();
    /*
     * Save old status
     */
    s_lvl = level;
    s_pur = purse;
    s_hp = pstats.s_hpt;
    s_str = pstats.s_str;
    s_exp = pstats.s_exp; 
    s_ac = (cur_armor != NULL ? cur_armor->o_ac : pstats.s_arm);
    s_hungry = hungry_state;
    s_lang_is_japanese = jrogue_lang_is_japanese;
    move(oy, ox);
}

/*
 * wait_for
 *	Sit around until the guy types the right key
 */



void wait_for(char ch)
{
    w_wait_for(stdscr, ch);
}

void w_wait_for(WINDOW * win, char ch)
{
    register char c;

    /* The --More-- and window-dismiss prompts need a visible cursor. */
    jrogue_show_cursor(TRUE);
    if (ch == '\n')
        while ((c = readcharw(win)) != '\n' && c != '\r')
	    continue;
    else
        while (readcharw(win) != ch)
	    continue;
    jrogue_restore_cursor();
}

/*
 * show_win:
 *	Function used to display a window and wait before returning
 */
void show_win(WINDOW * scr, char * message)
{
    mvwaddstr(scr, 0, 0, message);
    touchwin(scr);
#ifndef _WIN32
    wmove(scr, hero.y, hero.x);
#endif
    wrefresh(scr);
    w_wait_for(scr,' ');
    clearok(curscr, TRUE);
    touchwin(stdscr);
}
