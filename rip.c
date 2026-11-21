/*
 * File for the fun ends
 * Death or a total win
 *
 * @(#)rip.c	4.28 (Berkeley) 4/12/82
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include "rogue.h"
#include "jec/color.h"
#ifdef _WIN32
#include <io.h>
#include "windows/rogue_windows.h"
#else
#include <unistd.h>
#endif

static char *rip[] = {
"                       __________",
"                      /          \\",
JSTR_STATIC("                     /            \\", "                     /    REST    \\"),
JSTR_STATIC("                    /              \\", "                    /      IN      \\"),
JSTR_STATIC("                   /                \\", "                   /     PEACE      \\"),
"                  /                  \\",
"                  |                  |",
"                  |                  |",
JSTR_STATIC("                  |                  |", "                  |   killed by a    |"),
"                  |                  |",
"                  |       1980       |",
"                 *|     *  *  *      | *",
"         ________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______",
    0
};

void
jrogue_apply_rip_language(void)
{
    rip[2] = (char *) JSTR("                     /            \\", "                     /    REST    \\");
    rip[3] = (char *) JSTR("                    /              \\", "                    /      IN      \\");
    rip[4] = (char *) JSTR("                   /                \\", "                   /     PEACE      \\");
    rip[8] = (char *) JSTR("                  |                  |", "                  |   killed by a    |");
}

static void
grave_bounds(int row, int *left, int *right)
{
    switch (row)
    {
	case 9:  *left = 23; *right = 32; break;
	case 10: *left = 22; *right = 33; break;
	case 11: *left = 21; *right = 34; break;
	case 12: *left = 20; *right = 35; break;
	case 13: *left = 19; *right = 36; break;
	default: *left = 19; *right = 36; break;
    }
}

static void
grave_text(int row, const char *text)
{
    int left, right, width, len, col, i;
    char clip[PRBUF_SIZE];

    grave_bounds(row, &left, &right);
    width = right - left + 1;
    utf8_display_truncate(clip, text, width, sizeof clip);
    len = rogue_display_width(clip);
    col = left + (width - len) / 2;
    if (col < left)
	col = left;
    move(row, left);
    for (i = 0; i < width; i++)
	addch(' ');
    mvaddstr(row, col, clip);
}

/* Score output is written after endwin(), through stdio instead of
 * curses.  Some terminals retain LF-only output (ONLCR disabled), so a
 * plain '\n' advances to the next row without returning to column zero.
 * Supply CRLF explicitly for an interactive terminal, but keep ordinary
 * newline characters in redirected score output and score data files.
 */
static void
score_newline(int terminal)
{
    if (terminal)
        putchar('\r');
    putchar('\n');
}

/* Leave the last terminal column unused to avoid automatic line wrapping.
 * Unlike the old tab-separated printf, this formatter reserves space for
 * the cause of death before shortening an unusually long UTF-8 name. */
static void
score_print_row(unsigned long rank, unsigned int points, const char *name,
                const char *description, const char *annotation,
                int punctuation)
{
    char prefix[48];
    char shown_name[MAXSTR];
    char suffix[PRBUF_SIZE];
    char shown_suffix[PRBUF_SIZE];
    int budget, name_cols;

    snprintf(prefix, sizeof prefix, "%2lu %5u ", rank, points);
    if (annotation != NULL)
        snprintf(suffix, sizeof suffix, "%s%s", description, annotation);
    else
        snprintf(suffix, sizeof suffix, "%s", description);
    budget = 79 - rogue_display_width(prefix) - (punctuation ?
             (jrogue_lang_is_japanese ? 2 : 1) : 0);
    if (budget < 0)
        budget = 0;
    utf8_display_truncate(shown_suffix, suffix, budget, sizeof shown_suffix);
    name_cols = budget - rogue_display_width(shown_suffix);
    if (name_cols < 0)
        name_cols = 0;
    utf8_display_truncate(shown_name, name, name_cols, sizeof shown_name);

    fputs(prefix, stdout);
    fputs(shown_name, stdout);
    fputs(shown_suffix, stdout);
    if (punctuation)
        fputs(jrogue_lang_is_japanese ? "。" : ".", stdout);
}

/* The score format and its ten entries are shared by gameplay, -s and
 * the wizard score editor.  Keep the array static as in the original game. */
struct sc_ent {
    char sc_name[MAXSTR];
    unsigned int sc_flags;
    unsigned int sc_uid;
    unsigned short sc_monster;
    unsigned short sc_score;
    unsigned short sc_level;
};
static struct sc_ent top_ten[10];

/* Called only while the OS score lock is held.  All writes and the final
 * stdio flush must finish before another process can read the scoreboard. */
static void
score_write_locked(FILE *outf)
{
    char scoreline[101];
    void (*old_handler)(int);
    int i, write_error = FALSE;

    old_handler = signal(SIGINT, SIG_IGN);
    if (fseek(outf, 0L, SEEK_SET) != 0)
        write_error = TRUE;
    else
    {
        for (i = 0; i < 10; ++i)
        {
            encwrite(top_ten[i].sc_name, MAXSTR, outf);
            /* Keep the historical 100-byte encrypted record format. */
            memset(scoreline, 0, 100);
            snprintf(scoreline, 100, " %d %d %hd %hd %hd \n",
                     (int)top_ten[i].sc_flags,
                     (int)top_ten[i].sc_uid,
                     (short)top_ten[i].sc_monster,
                     (short)top_ten[i].sc_score,
                     (short)top_ten[i].sc_level);
            encwrite(scoreline, 100, outf);
        }
        if (fflush(outf) != 0 || ferror(outf))
            write_error = TRUE;
    }
    if (write_error)
    {
        fprintf(stderr, "%s\n",
                JSTR("スコアを書き込めませんでした。",
                     "Could not save the score."));
        fflush(stderr);
    }
    signal(SIGINT, old_handler);
}

/*
 * score:
 *	Figure score and post it.
 */
/* VARARGS2 */
void score(int amount, int flags, char monst)
{
    register struct sc_ent *scp;
    register int i;
    register struct sc_ent *sc2;
    register FILE *outf;
    register int prflags = 0;
    register int uid;
    /* On disk, each score metadata block is exactly 100 bytes. */
    char scoreline[101];
    int score_color = FALSE;
    int score_tty = FALSE;
    int highlighted;
    int score_locked = FALSE;
    char description[PRBUF_SIZE];
    char annotation[PRBUF_SIZE];
    const char *annotation_ptr;

    static char *reason[] = {
	"killed",
	"quit",
	"A total winner",
    };

    start_score();

    if (fd < 0)
        return;
    outf = md_fdopen(fd, "wb");
    if (outf == NULL)
    {
        /* Avoid passing a NULL FILE* to fseek()/fclose() when fdopen()
         * fails (for example when the process runs out of descriptors). */
        fprintf(stderr, "%s\n",
                JSTR("スコアファイルを開けませんでした。",
                     "Could not open score file stream."));
        close(fd);
        fd = -1;
#ifdef _WIN32
        jrogue_windows_wait_for_score_exit();
#endif
        return;
    }

    for (scp = top_ten; scp <= &top_ten[9]; scp++)
    {
	scp->sc_score = 0;
	for (i = 0; i < MAXSTR; i++)
	    scp->sc_name[i] = rnd(255);
	scp->sc_flags = RN;
	scp->sc_level = RN;
	scp->sc_monster = RN;
	scp->sc_uid = RN;
    }

    signal(SIGINT, SIG_DFL);
    if (flags != -1
#ifdef WIZARD
	    || wizard
#endif
	)
    {
	mvaddstr(LINES - 1, 0 , JSTR("［リターンを押してください］", "[Press return to continue]"));
        refresh();
	prbuf[0] = '\0';
        (void) get_str_size(prbuf, stdscr, PRBUF_SIZE);
        move(LINES - 1, 0);
        clrtoeol();
        refresh();
        endwin();
    }
#ifdef WIZARD
    if (wizard)
    {
	if (strcmp(prbuf, "names") == 0)
	    prflags = 1;
	else if (strcmp(prbuf, "edit") == 0)
	    prflags = 2;
    }
#endif
    /* Read, rank and update under one OS lock.  Acquiring it only at the
     * final write allowed simultaneous games to overwrite each other's
     * new scores.  A read-only -s also takes the lock for consistency. */
    score_locked = lock_sc();
    if (!score_locked)
    {
        fclose(outf);
#ifdef _WIN32
        jrogue_windows_wait_for_score_exit();
#endif
        return;
    }
    for (i = 0; i < 10; i++)
    {
        int saved_flags, saved_uid;
        short saved_monster, saved_score, saved_level;

        /* A score record consists of MAXSTR encrypted name bytes followed
         * by 100 encrypted metadata bytes.  Partial/corrupt records must
         * not be parsed as uninitialized text or leak into the ranking.
         * The on-disk format and its signed legacy fields stay unchanged. */
        if (encread(top_ten[i].sc_name, MAXSTR, fd) != MAXSTR)
            break;
        top_ten[i].sc_name[MAXSTR - 1] = '\0';
        if (encread(scoreline, 100, fd) != 100)
            break;
        scoreline[100] = '\0';
        if (sscanf(scoreline, "%d %d %hd %hd %hd",
                   &saved_flags, &saved_uid, &saved_monster,
                   &saved_score, &saved_level) != 5)
            break;
        top_ten[i].sc_flags = (unsigned int)saved_flags;
        top_ten[i].sc_uid = (unsigned int)saved_uid;
        top_ten[i].sc_monster = (unsigned short)saved_monster;
        top_ten[i].sc_score = (unsigned short)saved_score;
        top_ten[i].sc_level = (unsigned short)saved_level;
    }
    /*
     * Insert her in list if need be
     */
    sc2 = NULL;
    if (!noscore)
    {
	uid = md_getuid();

	for (scp = top_ten; scp <= &top_ten[9]; scp++)
	    if (amount > scp->sc_score)
		break;
#ifdef LIMIT_TOPTEN
            else if (flags != 2 && scp->sc_uid == uid && scp->sc_flags != 2)
		scp = &top_ten[9] + 1;	/* only one score per nowin uid */
#endif
	if (scp <= &top_ten[9])
	{
#ifdef LIMIT_TOPTEN
	    if (flags != 2)
		for (sc2 = scp; sc2 <= &top_ten[9]; sc2++)
		{
		    if (sc2->sc_uid == uid && sc2->sc_flags != 2)
			break;
		}
	    else
#endif
		sc2 = &top_ten[9];
	    while (sc2 > scp)
	    {
		*sc2 = sc2[-1];
		sc2--;
	    }
	    scp->sc_score = amount;
	    utf8_strlcpy(scp->sc_name, whoami, MAXSTR);
	    scp->sc_flags = flags;
	    if (flags == 2)
		scp->sc_level = max_level;
	    else
		scp->sc_level = level;
	    scp->sc_monster = monst;
	    scp->sc_uid = uid;
	    sc2 = scp;
	}
    }
    /* Ordinary gameplay and -s need no further edits: commit and release
     * the OS lock BEFORE terminal output (which may block or be slow).
     * The wizard's interactive delete mode is the one exception; it must
     * retain the lock until the requested edits have been applied. */
    if (prflags != 2)
    {
        if (sc2 != NULL)
            score_write_locked(outf);
        unlock_sc();
        score_locked = FALSE;
        fclose(outf);
        outf = NULL;
    }
    /*
     * Print the list
     */
    if (flags != -1)
        rogue_clear_plain_screen();
#ifdef _WIN32
    score_tty = _isatty(_fileno(stdout));
    score_color = (flags != -1 && color_mode && score_tty &&
                   jrogue_windows_score_vt());
#else
    score_tty = isatty(STDOUT_FILENO);
    score_color = (flags != -1 && color_mode && score_tty);
#endif
    /* Keep the score heading, column labels, and ordinary rows in the
     * terminal's normal foreground/background colors.  Only the highlighted
     * score row gets a color background when color mode is enabled. */
    fputs(jrogue_lang_is_japanese ? "トップ10の冒険者:" :
          "Top Ten Rogueists:", stdout);
    score_newline(score_tty);
    fputs(jrogue_lang_is_japanese ? "順位  得点  名前" :
          "Rank  Score Name", stdout);
    score_newline(score_tty);
    for (scp = top_ten; scp <= &top_ten[9]; scp++)
    {
        if (!scp->sc_score)
            break;

        /* Compose the fixed description before the user-defined name.
         * Only the displayed name is shortened; score data is untouched. */
        if (jrogue_lang_is_japanese)
        {
            if (scp->sc_flags == 0)
                snprintf(description, sizeof description, ": 地下%u階にて%s%s",
                         (unsigned)scp->sc_level,
                         killname((char)scp->sc_monster, FALSE),
                         isupper((unsigned char)scp->sc_monster) ?
                         "と戦いて死す" : "により死す");
            else if (scp->sc_flags == 1)
                snprintf(description, sizeof description, ": 地下%u階にて逃亡す",
                         (unsigned)scp->sc_level);
            else
                snprintf(description, sizeof description, ": 完全勝利者");
        }
        else
        {
            const char *why = scp->sc_flags < 3 ?
                              reason[scp->sc_flags] : reason[0];
            if (scp->sc_flags == 0)
                snprintf(description, sizeof description, ": %s on level %u by %s",
                         why, (unsigned)scp->sc_level,
                         killname((char)scp->sc_monster, TRUE));
            else
                snprintf(description, sizeof description, ": %s on level %u",
                         why, (unsigned)scp->sc_level);
        }

        annotation_ptr = NULL;
        if (prflags == 1)
        {
            char *realname = md_getusername(scp->sc_uid);
            if (realname == NULL)
                snprintf(annotation, sizeof annotation, " (%u)", scp->sc_uid);
            else
                snprintf(annotation, sizeof annotation, " (%s)", realname);
            annotation_ptr = annotation;
        }
        /* Highlight the newly recorded score in both display modes.
         * Color selects the highlight style, not whether it is shown.
         * Keep redirected score output free of terminal escapes. */
        highlighted = score_tty && scp == sc2;
        if (highlighted)
        {
            if (score_color)
                /* Same light-blue highlight as Rogue 5.4.5 JEC. */
                fputs("\033[0m\033[48;5;153m\033[30m", stdout);
            else
            {
#ifdef _WIN32
                /* Legacy console host without VT uses Win32 attributes. */
                if (jrogue_windows_score_vt())
                    fputs("\033[7m", stdout);
                else
                    md_raw_standout();
#else
                fputs("\033[7m", stdout);
#endif
            }
        }
        score_print_row((unsigned long)(scp - top_ten + 1),
                        scp->sc_score, scp->sc_name, description,
                        annotation_ptr, prflags == 0);
        if (highlighted)
        {
            /* End highlighting before the next row; no style leaks. */
            /* Restore the terminal's normal colors after either style.
             * In particular, do not paint ordinary score rows in 254. */
#ifdef _WIN32
            if (!jrogue_windows_score_vt())
                md_raw_standend();
            else
#endif
                fputs("\033[0m", stdout);
        }
        if (prflags != 2)
            score_newline(score_tty);
        if (prflags == 2)
        {
            fflush(stdout);
            if (fgets(prbuf, PRBUF_SIZE, stdin) != NULL && prbuf[0] == 'd')
            {
                for (sc2 = scp; sc2 < &top_ten[9]; sc2++)
                    *sc2 = *(sc2 + 1);
                top_ten[9].sc_score = 0;
                for (i = 0; i < MAXSTR; i++)
                    top_ten[9].sc_name[i] = rnd(255);
                top_ten[9].sc_flags = RN;
                top_ten[9].sc_level = RN;
                top_ten[9].sc_monster = RN;
                scp--;
            }
        }
    }
    /* The wizard editor may change the table after displaying each row.
     * Only that mode keeps the lock across the interactive display. */
    if (prflags == 2)
    {
        if (sc2 != NULL)
            score_write_locked(outf);
        unlock_sc();
        score_locked = FALSE;
        fclose(outf);
    }
#ifdef _WIN32
    /* Explorer-launched consoles would otherwise disappear immediately. */
    jrogue_windows_wait_for_score_exit();
#endif
}

/*
 * death:
 *	Do something really fun when he dies
 */
void death(char monst)
{
    register char **dp = rip, *killer;
    register struct tm *lt;
    time_t date;
    char buf[PRBUF_SIZE];

    signal(SIGINT, SIG_IGN);
    purse -= purse / 10;
    signal(SIGINT, leave);
    time(&date);
    lt = localtime(&date);
    clear();
    move(8, 0);
    jrogue_color_special_attr(JROGUE_PAIR_TOMBSTONE);
    while (*dp)
        printw("%s\n", *dp++);
    if (jrogue_color_available())
    {
        /* The stone keeps its muted purple-gray color (xterm 59).
         * Grave flowers and ground use the palette-C color pairs (pale
         * lavender / muted blue-gray green); neither pair overlaps the
         * A..Z monster colors. */
        jrogue_color_special_attr(JROGUE_PAIR_GRAVE_FLOWERS);
        mvaddch(19, 17, '*');
        mvaddch(19, 24, '*');
        mvaddch(19, 27, '*');
        mvaddch(19, 30, '*');
        mvaddch(19, 39, '*');
        jrogue_color_special_attr(JROGUE_PAIR_GRAVE_GROUND);
        mvaddstr(20, 9, "________)/\\\\_//(\\/(/\\)/\\//\\/");
        mvaddstr(20, 38, "_)_______");
    }
    jrogue_color_normal_attr();
    if (jrogue_lang_is_japanese)
    {
	grave_text(11, "安らかに");
	grave_text(12, "眠れ");
    }
    grave_text(14, whoami);
    sprintf(buf, JSTR("＄%d", "%d Au"), purse);
    grave_text(15, buf);
    killer = killname(monst, FALSE);
    if (jrogue_lang_is_japanese)
    {
	grave_text(16, killer);
	grave_text(17, isupper((unsigned char) monst) ? "と戦いて死す" : "により死す");
    }
    else
    {
	grave_text(17, killer);
	if (monst == 's')
	    mvaddch(16, 32, ' ');
	else
	    mvaddstr(16, 33, vowelstr(killer));
    }
    snprintf(prbuf, PRBUF_SIZE, "%4d", 1900 + lt->tm_year);
    mvaddstr(18, 26, prbuf);
    move(LINES-1, 0);
    refresh();
    score(purse, 0, monst);
    exit(0);
}

/*
 * total_winner:
 *	Code for a winner
 */
void total_winner(void)
{
    register THING *obj;
    register int worth = 0;
    register char c;
    register int oldpurse;

    clear();
    if (jrogue_color_available())
        jrogue_color_special_attr(JROGUE_PAIR_WIN_GREEN);
    else
        standout();
    addstr("                                                               \n");
    addstr("  @   @               @   @           @          @@@  @     @  \n");
    addstr("  @   @               @@ @@           @           @   @     @  \n");
    addstr("  @   @  @@@  @   @   @ @ @  @@@   @@@@  @@@      @  @@@    @  \n");
    addstr("   @@@@ @   @ @   @   @   @     @ @   @ @   @     @   @     @  \n");
    addstr("      @ @   @ @   @   @   @  @@@@ @   @ @@@@@     @   @     @  \n");
    addstr("  @   @ @   @ @  @@   @   @ @   @ @   @ @         @   @  @     \n");
    addstr("   @@@   @@@   @@ @   @   @  @@@@  @@@@  @@@     @@@   @@   @  \n");
    addstr("                                                               \n");
    if (jrogue_color_available())
        jrogue_color_special_attr(JROGUE_PAIR_WIN_YELLOW);
    addstr(JSTR("     おめでとう！  君はようやく太陽の光のもとにたどりついた。  \n", "     Congratulations, you have made it to the light of day!    \n"));
    if (jrogue_color_available())
        jrogue_color_normal_attr();
    else
        standend();
    if (jrogue_color_available())
        jrogue_color_special_attr(JROGUE_PAIR_WIN_YELLOW);
    addstr(JSTR("\n     そして、運命の洞窟から生きて戻った精鋭の一人となった。    \n", "\nYou have joined the elite ranks of those who have escaped the\n"));
    addstr(JSTR("     君は故郷へ帰り、すべての戦利品を売って大金を手に入れ、    \n", "Dungeons of Doom alive.  You journey home and sell all your loot at\n"));
    addstr(JSTR("     戦士ギルドに迎え入れられたということである・・・。        \n", "a great profit and are admitted to the fighters guild.\n"));
    jrogue_color_normal_attr();
    mvaddstr(LINES - 1, 0, JSTR("--スペースを押してください--", "--Press space to continue--"));
    refresh();
    wait_for(' ');
    clear();
    mvaddstr(0, 0, JSTR("    価値  持ちもの", "   Worth  Item"));
    oldpurse = purse;
    for (c = 'a', obj = pack; obj != NULL; c++, obj = next(obj))
    {
	switch (obj->o_type)
	{
	    case FOOD:
		worth = 2 * obj->o_count;
	    when WEAPON:
		switch (obj->o_which)
		{
		    case MACE: worth = 8;
		    when SWORD: worth = 15;
		    when CROSSBOW: worth = 30;
		    when ARROW: worth = 1;
		    when DAGGER: worth = 2;
		    when TWOSWORD: worth = 75;
		    when DART: worth = 1;
		    when BOW: worth = 15;
		    when BOLT: worth = 1;
		    when SPEAR: worth = 5;
		}
		worth *= 3 * (obj->o_hplus + obj->o_dplus) + obj->o_count;
		obj->o_flags |= ISKNOW;
	    when ARMOR:
		switch (obj->o_which)
		{
		    case LEATHER: worth = 20;
		    when RING_MAIL: worth = 25;
		    when STUDDED_LEATHER: worth = 20;
		    when SCALE_MAIL: worth = 30;
		    when CHAIN_MAIL: worth = 75;
		    when SPLINT_MAIL: worth = 80;
		    when BANDED_MAIL: worth = 90;
		    when PLATE_MAIL: worth = 150;
		}
		worth += (9 - obj->o_ac) * 100;
		worth += (10 * (a_class[obj->o_which] - obj->o_ac));
		obj->o_flags |= ISKNOW;
	    when SCROLL:
		worth = s_magic[obj->o_which].mi_worth;
		worth *= obj->o_count;
		if (!s_know[obj->o_which])
		    worth /= 2;
		s_know[obj->o_which] = TRUE;
	    when POTION:
		worth = p_magic[obj->o_which].mi_worth;
		worth *= obj->o_count;
		if (!p_know[obj->o_which])
		    worth /= 2;
		p_know[obj->o_which] = TRUE;
	    when RING:
		worth = r_magic[obj->o_which].mi_worth;
		if (obj->o_which == R_ADDSTR || obj->o_which == R_ADDDAM ||
		    obj->o_which == R_PROTECT || obj->o_which == R_ADDHIT)
		{
		    if (obj->o_ac > 0)
			worth += obj->o_ac * 100;
		    else
			worth = 10;
		}
		if (!(obj->o_flags & ISKNOW))
		    worth /= 2;
		obj->o_flags |= ISKNOW;
		r_know[obj->o_which] = TRUE;
	    when STICK:
		worth = ws_magic[obj->o_which].mi_worth;
		worth += 20 * obj->o_charges;
		if (!(obj->o_flags & ISKNOW))
		    worth /= 2;
		obj->o_flags |= ISKNOW;
		ws_know[obj->o_which] = TRUE;
	    when AMULET:
		worth = 1000;
	}
	if (worth < 0)
	    worth = 0;
	{
	    char itembuf[PRBUF_SIZE];
	    utf8_display_truncate(itembuf, inv_name(obj, FALSE), COLS - 11, sizeof itembuf);
	    mvprintw(c - 'a' + 1, 0, "%c) %5d  %s", c, worth, itembuf);
	}
	purse += worth;
    }
    mvprintw(c - 'a' + 1, 0, JSTR("   %5d  金塊          ", "   %5d  Gold Pieces          "), oldpurse);
    refresh();
    score(purse, 2, 0);
    exit(0);
}

/*
 * killname:
 *	Convert a code to a monster name
 */
char * killname(char monst, bool doart)
{
    register const char *sp;
    register bool article;

    sp = prbuf;
    article = TRUE;
    switch (monst)
    {
	case 'a':
	    sp = JSTR("矢", "arrow");
	when 'b':
	    sp = JSTR("ボルト", "bolt");
	when 'd':
	    sp = JSTR("毒投げ矢", "dart");
	when 's':
	    sp = JSTR("飢え", "starvation");
	    article = FALSE;
	otherwise:
	    if (monst >= 'A' && monst <= 'Z')
		sp = monsters[monst-'A'].m_name;
	    else
	    {
		sp = JSTR("神", "God");
		article = FALSE;
	    }
    }
    if (jrogue_lang_is_japanese)
    {
	utf8_strlcpy(prbuf, sp, PRBUF_SIZE);
	return prbuf;
    }
    if (doart && article)
	snprintf(prbuf, PRBUF_SIZE, "a%s ", vowelstr(sp));
    else
	prbuf[0] = '\0';
    snprintf(prbuf + strlen(prbuf), PRBUF_SIZE - strlen(prbuf), "%s", sp);
    utf8_trim_partial(prbuf);
    return prbuf;
}
