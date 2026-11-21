/*
 * Various installation dependent routines
 *
 * @(#)mach_dep.c	4.23 (Berkeley) 5/19/82
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

/*
 * The various tuneable defines are:
 *
 *	SCOREFILE	Where/if the score file should live.
 *	MAXLOAD		What (if any) the maximum load average should be
 *			when people are playing.  If defined, then
 *	LOADAV		Should rogue define it's own routine to
 *			get the load average?
 *	NAMELIST	If so, where does the system namelist hide?
 *	MAXUSERS	What (if any) the maximum user count should be
 *			when people are playing.  If defined, then
 *	UCOUNT		Should rogue define it's own routine to
 *			count users?
 *	UTMP		If so, where does the user list hide?
 *	CHECKTIME	How often/if rogue should check during the game
 *			for high load average.
 */

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <curses.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>
#ifdef _WIN32
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>
#include "rogue.h"

int num_checks;		/* times we've gone over in checkout() */

/* The score file is locked by the operating system, not by a stale-prone
 * separate .lck file.  The lock is automatically released on process exit. */
#ifdef SCOREFILE
static int score_lock_held;
#endif

/*
 * init_check:
 *	Check out too see if it is proper to play the game now
 */
void init_check(void)
{
    if (too_much())
    {
	printf("Sorry, %s, but the system is too loaded now.\n", whoami);
	printf("Try again later.  Meanwhile, why not enjoy a%s %s?\n",
	    vowelstr(fruit), fruit);
	if (author())
	    printf("However, since you're a good guy, it's up to you\n");
	else
	    exit(1);
    }
}

/*
 * open_score:
 *	Open up the score file for future use, and then
 *	setuid(getuid()) in case we are running setuid.
 */
void open_score(void)
{
#ifdef SCOREFILE
    fd = md_file_open(SCOREFILE, O_RDWR | O_CREAT, 0666);
#else
    fd = -1;
#endif
    md_normaluser();
}

/*
 * setup:
 *	Get starting setup for all games
 */
void setup(void)
{
#ifdef CHECKTIME
    int  checkout();
#endif

    /*
     * make sure that large terminals don't overflow the bounds
     * of the program
     */
    if (LINES > MAXLINES)
	LINES = MAXLINES;
    if (COLS > MAXCOLS)
	COLS = MAXCOLS;

#ifdef SIGHUP
    signal(SIGHUP, auto_save);
#endif
#ifndef DUMP
    signal(SIGILL, crash_exit);
#ifdef SIGTRAP
    signal(SIGTRAP, crash_exit);
#endif
#ifdef SIGIOT
    signal(SIGIOT, crash_exit);
#endif
#ifdef SIGEMT
    signal(SIGEMT, crash_exit);
#endif
    signal(SIGFPE, crash_exit);
#ifdef SIGBUS
    signal(SIGBUS, crash_exit);
#endif
    signal(SIGSEGV, crash_exit);
#ifdef SIGSYS
    signal(SIGSYS, crash_exit);
#endif
    signal(SIGTERM, auto_save);
#endif

    signal(SIGINT, quit);
#ifndef DUMP
#ifdef SIGQUIT
    signal(SIGQUIT, endit);
#endif
#endif
#ifdef CHECKTIME
    signal(SIGALRM, checkout);
    alarm(CHECKTIME * 60);
    num_checks = 0;
#endif
    crmode();				/* Cbreak mode */
    noecho();				/* Echo off */
}

/*
 * start_score:
 *	Start the scoring sequence
 */
void start_score(void)
{
#ifdef SIGALRM
    signal(SIGALRM, SIG_IGN);
#endif
}

/*
 * issymlink:
 *	See if the file has a symbolic link
 */
int issymlink(char * sp)
{
#ifdef S_IFLNK
    struct stat sbuf2;

    if (lstat(sp, &sbuf2) < 0)
	return FALSE;
    else
	return ((sbuf2.st_mode & S_IFMT) != S_IFREG);
#else
    (void)sp;
    return FALSE;
#endif
}

/*
 * too_much:
 *	See if the system is being used too much for this game
 */
int too_much(void)
{
#ifdef MAXLOAD
    double avec[3];
    
    if (md_getloadavg(avec) == 0)
        if (avec[2] > (MAXLOAD / 10.0))
	    return(1);
#endif
#ifdef MAXUSERS
    if (md_ucount() > MAXUSERS)
	return(1) ;
#endif
    return(0);
}

/*
 * author:
 *	See if a user is an author of the program
 */
int author(void)
{
#ifdef WIZARD
    if (wizard)
	return TRUE;
#endif
    switch (md_getuid())
    {
	case 0:
	    return TRUE;
	default:
	    return FALSE;
    }
}

/*
 * checkout:
 *	Check each CHECKTIME seconds to see if the load is too high
 */
void checkout(int s)
{
    static char *msgs[] = {
	"The load is too high to be playing.  Please leave in %0.1f minutes",
	"Please save your game.  You have %0.1f minutes",
	"Last warning.  You have %0.1f minutes to leave",
    };
    int checktime = 0;

    (void)s;
#ifdef SIGALRM
    signal(SIGALRM, checkout);
#endif

    if (too_much())
    {
	if (author())
	{
	    num_checks = 1;
	    chmsg("The load is rather high, O exaulted one");
	}
	else if (num_checks++ == 3)
	    fatal("Sorry.  You took to long.  You are dead\n");

#ifdef CHECKTIME
	checktime = (CHECKTIME * 60) / num_checks;
#endif
#ifdef SIGALRM
	alarm(checktime);
#endif

	chmsg(msgs[num_checks - 1], ((double) checktime / 60.0));
    }
    else
    {
	if (num_checks)
	{
	    num_checks = 0;
	    chmsg("The load has dropped back down.  You have a reprieve");
	}
#ifdef CHECKTIME
#ifdef SIGALRM
	alarm(CHECKTIME * 60);
#endif
#endif
    }
}

/*
 * chmsg:
 *	checkout()'s version of msg.  If we are in the middle of a
 *	shell, do a printf instead of a msg to avoid the refresh.
 */
void chmsg(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if (in_shell)
    {
	vprintf(fmt, ap);
	putchar('\n');
	fflush(stdout);
    }
    else
    {
	doadd(fmt, ap);
	endmsg();
    }
    va_end(ap);
}

/*
 * Lock the scoreboard itself, before reading the existing entries.  A
 * separate lock file made simultaneous score updates unsafe: creat() is
 * neither exclusive nor an ownership test.  The kernel releases this lock
 * automatically if the game exits unexpectedly.
 *
 * Use a bounded five-second retry so a busy scoreboard never leaves the
 * game waiting at a prompt while still holding other resources.
 */
int lock_sc(void)
{
#ifdef SCOREFILE
    int attempt;

    if (score_lock_held)
        return TRUE;
    if (fd < 0)
        return FALSE;

    for (attempt = 0; attempt < 50; ++attempt)
    {
        if (md_score_try_lock(fd))
        {
            score_lock_held = TRUE;
            return TRUE;
        }
        if (errno != EACCES && errno != EAGAIN)
            break;
        if (attempt < 49)
            md_delay(100);
    }
    fprintf(stderr, "%s: %s\n",
            JSTR("スコアファイルをロックできませんでした",
                 "Could not lock score file"), strerror(errno));
    fflush(stderr);
    return FALSE;
#else
    return TRUE;
#endif
}

/* Release only a lock actually acquired by this process. */
void unlock_sc(void)
{
#ifdef SCOREFILE
    if (!score_lock_held)
        return;
    md_score_unlock(fd);
    score_lock_held = FALSE;
#endif
}

/*
 * flush_type:
 *	Flush typeahead for traps, etc.
 */
void flush_type(void)
{
    flushinp();
}
