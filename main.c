/*
 * #     #
 * #    #   #    #  #   ##  #    #   # 
 *		         #
 *
 * @(#)main.c	4.26 (Berkeley) 2/4/82
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <signal.h>
#include <time.h>
#ifdef _WIN32
#include <io.h>
#include <process.h>
#include "windows/rogue_windows.h"
#else
#include <unistd.h>
#endif
#include <limits.h>
#include <string.h>
#include <locale.h>
#include "rogue.h"
#include "jec/color.h"


/* Avoid silently truncating a UTF-8 save path into a different filename. */
static int
copy_path_parts(char *dst, size_t dstsize, const char *prefix,
                const char *suffix)
{
    size_t a, b;

    if (dstsize == 0 || prefix == NULL || suffix == NULL)
        return FALSE;
    a = strlen(prefix);
    b = strlen(suffix);
    if (a >= dstsize || b >= dstsize - a)
    {
        dst[0] = '\0';
        return FALSE;
    }
    memcpy(dst, prefix, a);
    memcpy(dst + a, suffix, b + 1);
    utf8_trim_partial(dst);
    return TRUE;
}

static int
expand_home_path(const char *src, char *dst, size_t dstsize)
{
    const char *homedir;

    if (src == NULL || dstsize == 0)
        return FALSE;
    if (src[0] == '~' && (src[1] == '\0' || src[1] == '/'
#ifdef _WIN32
                             || src[1] == '\\'
#endif
                             ))
    {
        homedir = home;
        if (homedir[0] == '\0')
            homedir = md_gethomedir();
        if (homedir == NULL)
            return FALSE;
        return copy_path_parts(dst, dstsize, homedir, src + 1);
    }
    return copy_path_parts(dst, dstsize, "", src);
}

static int
path_has_slash(const char *path)
{
 #ifdef _WIN32
    return path && (strchr(path, '/') != NULL || strchr(path, '\\') != NULL ||
                    strchr(path, ':') != NULL);
#else
    return path != NULL && strchr(path, '/') != NULL;
#endif
}

static int
join_save_path(const char *dir, const char *name, char *dst, size_t dstsize)
{
    char expanded_dir[PATHSTR];
    size_t len;

    if (dstsize == 0)
        return FALSE;
    dst[0] = '\0';
    if (name == NULL || name[0] == '\0')
        name = "rogue.save";
    if (path_has_slash(name) || name[0] == '~')
        return expand_home_path(name, dst, dstsize);
    if (dir == NULL || dir[0] == '\0')
        dir = home;
    if (!expand_home_path(dir, expanded_dir, sizeof expanded_dir))
        return FALSE;
    if (expanded_dir[0] == '\0')
        return copy_path_parts(dst, dstsize, "", name);
    len = strlen(expanded_dir);
    if (expanded_dir[len - 1] != '/'
#ifdef _WIN32
        && expanded_dir[len - 1] != '\\'
#endif
        )
    {
        if (len + 1 >= sizeof expanded_dir)
            return FALSE;
        expanded_dir[len] = '/';
        expanded_dir[len + 1] = '\0';
    }
    return copy_path_parts(dst, dstsize, expanded_dir, name);
}

/*
 * main:
 *	The main program, of course
 */
int main(int argc, char * * argv, char * * envp)
{
    register char *env;
    int lowtime;
#ifdef _WIN32
    char *selected_save = NULL;
    int windows_argument_index = 1;
#endif

#ifdef _WIN32
    /* UCRT's UTF-8 locale and PDCursesMod VT must be set before curses. */
    setlocale(LC_ALL, ".UTF-8");
#else
    setlocale(LC_ALL, "");
#endif
    md_init();
#ifdef _WIN32
    jrogue_windows_console_init();
    jrogue_windows_resize_direct_launch();
#endif
    jrogue_detect_language();

#ifndef DUMP
#ifdef SIGQUIT
    signal(SIGQUIT, exit);
#endif
    signal(SIGILL, exit);
#ifdef SIGTRAP
    signal(SIGTRAP, exit);
#endif
#ifdef SIGIOT
    signal(SIGIOT, exit);
#endif
#ifdef SIGEMT
    signal(SIGEMT, exit);
#endif
    signal(SIGFPE, exit);
#ifdef SIGBUS
    signal(SIGBUS, exit);
#endif
    signal(SIGSEGV, exit);
#ifdef SIGSYS
    signal(SIGSYS, exit);
#endif
#endif

#ifdef WIZARD
    /*
     * Check to see if he is a wizard
     */
    if (argc >= 2 && argv[1][0] == '\0')
    {
        char *wizard_pass = md_getpass(JSTR("魔法使いの合言葉は？", "Wizard's password: "));

        if (wizard_check_password(wizard_pass))
        {
            wizard = TRUE;
            player.t_flags |= SEEMONST;
            argv++;
            argc--;
#ifdef _WIN32
            windows_argument_index++;  /* Preserve the UTF-16 argv index. */
#endif
        }
    }
#endif

    /*
     * get home and options from environment
     */
    snprintf(home, PATHSTR, "%s", md_gethomedir());
    home[PATHSTR - 1] = '\0';
    utf8_trim_partial(home);
    if (!join_save_path(DEFAULT_SAVEDIR, DEFAULT_SAVEFILE, file_name, PATHSTR))
    {
        fputs(JSTR("セーブファイルのパスが長すぎます。\n",
                  "Save file path is too long.\n"), stderr);
        return 1;
    }

#ifdef _WIN32
    env = jrogue_windows_rogueopts_utf8();
#else
    env = getenv("ROGUEOPTS");
#endif
    if (env != NULL)
        parse_opts(env);
#ifdef _WIN32
    if (env != NULL)
        free(env);
#endif
    if (env == NULL || whoami[0] == '\0')
	strucpy(whoami, md_getusername(md_getuid()), strlen(md_getusername(md_getuid())));
    if (env == NULL || fruit[0] == '\0')
	utf8_strlcpy(fruit, JSTR("ネバネバかび", "slime-mold"), MAXSTR);
    jrogue_apply_language();

    /*
     * check for print-score option
     */
    open_score();
    if (argc == 2 && strcmp(argv[1], "-s") == 0)
    {
	noscore = TRUE;
	score(0, -1, 0);
	exit(0);
    }
    init_check();			/* check for legal startup */
#ifdef _WIN32
    /* Win32 main(argv) may use the system ANSI codepage.  Convert the
     * actual UTF-16 command line before interpreting a saved-game path. */
    if (argc == 2)
    {
        selected_save = jrogue_windows_argument_utf8(windows_argument_index);
        if (!selected_save)
        {
            fputs(JSTR("セーブファイル名を読み取れません。\n",
                      "Cannot read saved game filename.\n"), stderr);
            return 1;
        }
    }
    else if (argc == 1)
        selected_save = jrogue_windows_launch_save(file_name);
    if (selected_save)
    {
        if (strlen(selected_save) >= PATHSTR)
        {
            fputs(JSTR("セーブファイルのパスが長すぎます。\n",
                      "Save file path is too long.\n"), stderr);
            free(selected_save);
            return 1;
        }
        if (!restore(selected_save, envp))
        {
            free(selected_save);
            jrogue_windows_wait_for_restore_error();
            return 1;
        }
    }
#else
    if (argc == 2)
        if (!restore(argv[1], envp))
        {
            endwin();
            exit(1);
        }
#endif
    lowtime = (int) time(NULL);

#ifdef WIZARD
    noscore = wizard;
#endif
    if (getenv("SEED") != NULL)
    {
	dnum = atoi(getenv("SEED"));
	noscore = TRUE;
    }
    else
	dnum = lowtime + getpid();
#ifdef WIZARD
    if (wizard)
	printf(JSTR("やあ、%s。#%dのダンジョンへようこそ・・・", "Hello %s, welcome to dungeon #%d"), whoami, dnum);
    else
#endif
	printf(JSTR("やあ、%s。 ダンジョンを掘っている間、ちょっと待っててね...\n\n", "Hello %s, just a moment while I dig the dungeon...\n\n"), whoami);
    fflush(stdout);
    md_delay(ROGUE_STARTUP_DELAY_MS);
    seed = dnum;

    init_player();			/* Set up initial player stats */
    init_things();			/* Set up probabilities of things */
    init_names();			/* Set up names of scrolls */
    init_colors();			/* Set up colors of potions */
    init_stones();			/* Set up stone settings of rings */
    init_materials();			/* Set up materials of wands */

    initscr();				/* Start up cursor package */
#ifdef _WIN32
    jrogue_windows_retry_direct_launch_resize();
#endif
    jrogue_color_init();
#ifdef _WIN32
    /* Paint blank cells through PDCurses, not raw ANSI sequences. */
    if (jrogue_color_available())
    {
        erase();
        refresh();
    }
#endif
    jrogue_restore_cursor();

    if (COLS < 70) 
    { 
	printf("%s", JSTR("\n\n端末の横幅が足りません。\n", "\n\nSorry, but your terminal window has too few columns.\n"));
        printf(JSTR("この端末は %d 桁ですが、70 桁必要です。\n", "Your terminal has %d columns, needs 70.\n"), COLS); 
        endwin(); 
        exit(1); 
	} 

    if (LINES < 22) 
    { 
        printf("%s", JSTR("\n\n端末の行数が足りません。\n", "\n\nSorry, but your terminal window has too few lines.\n")); 
        printf(JSTR("この端末は %d 行ですが、22 行必要です。\n", "Your terminal has %d lines, needs 22.\n"), LINES); 
        endwin(); 
        exit(1); 
    } 
 
    if ((*whoami == '\0') || (strcmp(whoami,"dosuser")==0))
    {
        noecho();
        mpos = rogue_display_width(JSTR("冒険者の名前は？ ", "Rogue's Name? "));
        mvaddstr(23, 2, JSTR("冒険者の名前は？ ", "Rogue's Name? "));
        wmove(stdscr, 23, 2 + mpos);
        if (get_str(whoami, stdscr) == QUIT)
            whoami[0] = '\0';
    }

    if (*whoami == '\0')
        strcpy(whoami,JSTR("ロドニー", "Rodney"));

    setup();

    /*
     * Set up windows
     */
    hw = newwin(LINES, COLS, 0, 0);
    jrogue_color_apply_window(hw);
    keypad(stdscr,1);
    jrogue_restore_cursor();
    new_level();			/* Draw current level */
    /*
     * Start up daemons and fuses
     */
    rogue_daemon(doctor, 0, AFTER);
    fuse(swander, 0, WANDERTIME, AFTER);
    rogue_daemon(stomach, 0, AFTER);
    rogue_daemon(runners, 0, AFTER);
    playit();
    return 0;
}


/*
 * crash_exit:
 *	Minimal signal handler for fatal signals.  Do not call curses here.
 */
void crash_exit(int sig)
{
    static const char msg[] = "\nRogue caught a fatal signal; exiting.\n";

    signal(sig, SIG_DFL);
#ifdef _WIN32
    _write(_fileno(stderr), msg, (unsigned int)(sizeof msg - 1));
#else
    write(STDERR_FILENO, msg, sizeof msg - 1);
#endif
    _exit(128 + sig);
}

/*
 * endit:
 *	Exit the program abnormally.
 */
void endit(int a)
{
    (void)a;
    fatal(JSTR("そこまで終了したいなら、仕方ありません。\n", "Ok, if you want to exit that badly, I'll have to allow it\n"));
}

/*
 * fatal:
 *	Exit the program, printing a message.
 */
void fatal(char * s)
{
    clear();
    move(LINES-2, 0);
    printw("%s", s);
    refresh();
    jrogue_show_cursor(TRUE);
    endwin();
    (void) exit(0);
}

/*
 * rnd:
 *	Pick a very random number.
 */
int rnd(int range)
{
    return range == 0 ? 0 : abs((int) RN) % range;
}

/*
 * roll:
 *	Roll a number of dice
 */
int roll(int number, int sides)
{
    register int dtotal = 0;

    while (number--)
	dtotal += rnd(sides)+1;
    return dtotal;
}

/*
 * tstp:
 *	Handle stop and start signals
 */
void tstp(int a)
{
    register int y, x;
    register int oy, ox;

    (void)a;

    getyx(curscr, oy, ox);
    mvcur(0, COLS - 1, LINES - 1, 0);
    jrogue_show_cursor(TRUE);
    endwin();
    /* Give the calling shell its own colors while this process is stopped. */
    jrogue_color_terminal_restore();
    clearok(curscr, TRUE);
    fflush(stdout);
#ifdef SIGTSTP
    signal(SIGTSTP, SIG_DFL);
    kill(0, SIGTSTP);
    signal(SIGTSTP, tstp);
#endif
    crmode();
    noecho();
    clearok(curscr, TRUE);
    jrogue_color_terminal_resume();
    wrefresh(curscr);
    getyx(curscr, y, x);
    mvcur(y, x, oy, ox);
    fflush(stdout);
    wmove(curscr, oy, ox);
    jrogue_restore_cursor();
}

/*
 * playit:
 *	The main loop of the program.  Loop until the game is over,
 *	refreshing things and looking at the proper times.
 */
void playit(void)
{
    register char *opts;

    /*
     * set up defaults for slow terminals
     */

    if (baudrate() < 1200)
    {
        terse = TRUE;
        jump = TRUE;
    }

    /*
     * parse environment declaration of options
     */
#ifdef _WIN32
    opts = jrogue_windows_rogueopts_utf8();
#else
    opts = getenv("ROGUEOPTS");
#endif
    if (opts != NULL)
        parse_opts(opts);
#ifdef _WIN32
    free(opts);
#endif


    oldpos = hero;
    oldrp = roomin(&hero);
    while (playing)
	command();			/* Command execution */
    endit(0);
}

/*
 * quit:
 *	Have player make certain, then exit.
 */
void quit(int a)
{
    register int oy, ox;
    int answer;

    (void)a;

    /*
     * Reset the signal in case we got here via an interrupt
     */

    if (signal(SIGINT, quit) != quit)
	mpos = 0;
    getyx(curscr, oy, ox);
    msg(JSTR("本当に終了してよいのですか？ y/n ", "really quit?"));
    jrogue_show_cursor(TRUE);
    answer = readchar();
    jrogue_restore_cursor();
    if (answer == 'y')
    {
	signal(SIGINT, leave);
	clear();
	mvprintw(LINES - 2, 0, JSTR("所持金は＄%d", "You quit with %d gold pieces"), purse);
	move(LINES - 1, 0);
	refresh();
	score(purse, 1, 0);
	exit(0);
    }
    else
    {
	move(0, 0);
	clrtoeol();
	status();
	move(oy, ox);
	refresh();
	mpos = 0;
	count = 0;
    }
}

/*
 * leave:
 *	Leave quickly, but curteously
 */
void leave(int sig)
{
    (void)sig;
/*
    if (!_endwin)
    {*/
	mvcur(0, COLS - 1, LINES - 1, 0);
        jrogue_show_cursor(TRUE);
	endwin();
    /* } */
    putchar('\n');
    exit(0);
}

/*
 * shell:
 *	Let him escape for a while
 */
void shell(void)
{
    /*
     * Set the terminal back to original mode
     */
    move(LINES-1, 0);
    refresh();
    jrogue_show_cursor(TRUE);
    endwin();
    putchar('\n');
    putchar('\n');
    in_shell = TRUE;
    after = FALSE;

    jrogue_color_terminal_restore();
    md_shellescape();
    jrogue_color_terminal_resume();

    noecho();
    crmode();
    in_shell = FALSE;
    jrogue_restore_cursor();
    clearok(stdscr, TRUE);
    touchwin(stdscr);
}
