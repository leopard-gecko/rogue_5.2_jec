/*
 * save and restore routines
 *
 * @(#)save.c	4.15 (Berkeley) 5/10/82
 *
 * Rogue: Exploring the Dungeons of Doom
 * Copyright (C) 1980, 1981, 1982 Michael Toy, Ken Arnold and Glenn Wichman
 * All rights reserved.
 *
 * See the file LICENSE.TXT for full copyright and licensing information.
 */

#include <curses.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif
#include <errno.h>
#include <string.h>
#include <time.h>
#define KERNEL
#include <signal.h>
#undef KERNEL
#include "rogue.h"
#ifdef _WIN32
#include "windows/rogue_windows.h"
#endif
#include "jec/color.h"

typedef struct stat STAT;

#define SAVE_HEADER_MAGIC 0x52353202
/* Briefly keep the final save name visible before save_file() clears it. */
#define SAVE_FILENAME_DELAY_MS 350

extern char version[], encstr[];
extern bool _endwin;

STAT sbuf;

/*
 * Follow Rogue 3.6's language-dependent restore timestamp.
 * Japanese: 2026年11月8日（日）07:21:02 (local time)
 * English:  Sun Nov  8 07:21:02 2026 (original ctime form)
 * Use an explicit Japanese weekday table so the result does not depend on
 * the process locale or the platform's strftime() locale support.
 */
static void
format_restore_timestamp(char *dst, size_t dstsz, time_t mtime)
{
    static const char *weekdays[] = {
        "日", "月", "火", "水", "木", "金", "土"
    };
    const char *timestr;
    struct tm *local;

    if (dstsz == 0)
        return;
    dst[0] = '\0';
    if (jrogue_lang_is_japanese)
    {
        local = localtime(&mtime);
        if (local == NULL || local->tm_wday < 0 || local->tm_wday > 6)
        {
            snprintf(dst, dstsz, "%s", "日時不明");
            return;
        }
        snprintf(dst, dstsz, "%d年%d月%d日（%s）%02d:%02d:%02d",
                 local->tm_year + 1900, local->tm_mon + 1, local->tm_mday,
                 weekdays[local->tm_wday], local->tm_hour, local->tm_min,
                 local->tm_sec);
    }
    else
    {
        timestr = ctime(&mtime);
        snprintf(dst, dstsz, "%s", timestr != NULL ? timestr : "unknown time");
        dst[strcspn(dst, "\n")] = '\0';
    }
    dst[dstsz - 1] = '\0';
    utf8_trim_partial(dst);
}

/*
 * Restore is displayed on the top (message) row.  Keep a spare terminal
 * column: curses may advance to the next row when the last column is filled.
 * If the path is long, preserve its end (especially the save-file basename)
 * and the full modification timestamp instead of allowing it to wrap over
 * the dungeon.  Width is measured in screen cells, not UTF-8 bytes.
 */
static void
format_restore_message(char *dst, size_t dstsz, const char *file,
                       time_t mtime, int screen_cols)
{
    const char *tail;
    char tbuf[64];
    char composed[PATHSTR + 80];
    int limit, pathcols;
    int shortened = FALSE;

    if (dstsz == 0)
        return;
    dst[0] = '\0';
    if (file == NULL)
        file = "";

    format_restore_timestamp(tbuf, sizeof tbuf, mtime);

    /* setup() caps the game's columns even if the terminal is wider. */
    limit = screen_cols;
    if (limit > MAXCOLS)
        limit = MAXCOLS;
    --limit;                 /* never write into the final terminal cell */
    if (limit <= 0)
        return;

    /* Reserve ': ' and the full timestamp; ellipsize only the path. */
    pathcols = limit - 2 - rogue_display_width(tbuf);
    tail = file;
    if (pathcols > 3 && rogue_display_width(file) > pathcols)
    {
        shortened = TRUE;
        while (*tail && rogue_display_width(tail) > pathcols - 3)
        {
            ++tail;
            /* Do not start the retained suffix inside a UTF-8 sequence. */
            while (*tail && (((unsigned char)*tail & 0xc0) == 0x80))
                ++tail;
        }
    }

    snprintf(composed, sizeof composed, "%s%s: %s",
             shortened ? "..." : "", tail, tbuf);
    composed[sizeof composed - 1] = '\0';
    utf8_trim_partial(composed);
    utf8_display_truncate(dst, composed, limit, (int)dstsz);
}

/*
 * Keep the save filename/confirmation on the message line.  Preserve the
 * path's end when it is too long; the actual path used for saving is not
 * shortened.  Reserve the final screen column to avoid a terminal wrap.
 */
static void
format_save_path_message(char *dst, size_t dstsz, const char *prefix,
                         const char *path, const char *suffix,
                         int screen_cols)
{
    const char *tail;
    char composed[PATHSTR + 128];
    int limit, pathcols, shortened = FALSE;

    if (dstsz == 0)
        return;
    dst[0] = '\0';
    if (path == NULL)
        path = "";
    limit = screen_cols;
    if (limit > MAXCOLS)
        limit = MAXCOLS;
    --limit;
    if (limit <= 0)
        return;

    pathcols = limit - rogue_display_width(prefix) -
                     rogue_display_width(suffix);
    if (pathcols < 0)
        pathcols = 0;
    tail = path;
    if (pathcols > 3 && rogue_display_width(path) > pathcols)
    {
        shortened = TRUE;
        while (*tail && rogue_display_width(tail) > pathcols - 3)
        {
            ++tail;
            while (*tail && (((unsigned char)*tail & 0xc0) == 0x80))
                ++tail;
        }
    }
    snprintf(composed, sizeof composed, "%s%s%s%s", prefix,
             shortened ? "..." : "", tail, suffix);
    composed[sizeof composed - 1] = '\0';
    utf8_trim_partial(composed);
    utf8_display_truncate(dst, composed, limit, (int)dstsz);
}

static void
format_save_confirmation(char *dst, size_t dstsz, const char *path,
                         int screen_cols)
{
    format_save_path_message(dst, dstsz,
                             JSTR("セーブファイル (", "save file ("), path,
                             JSTR(")？ ", ")? "), screen_cols);
}

/*
 * Older saves can contain damage to the top wall of a room if a long save
 * prompt wrapped onto dungeon row 1 before the screen was serialized.
 * Repair ONLY the wall of a room known to be already visible: a wall drawn
 * on row 2 is evidence that the room was revealed.  Never redraw the full
 * _level row -- that would disclose rooms which the player has not seen.
 */
static void
repair_saved_top_room_walls(void)
{
    int i, x, left, right;
    struct room *rp;

    if (LINES < 4)
        return;
    for (i = 0; i < MAXROOMS; ++i)
    {
        rp = &rooms[i];
        if ((rp->r_flags & ISGONE) || rp->r_pos.y != 1)
            continue;
        left = rp->r_pos.x;
        right = left + rp->r_max.x - 1;
        if (left <= 0 || right >= COLS - 1 || left >= right)
            continue;
        if (CCHAR(mvinch(2, left)) != '|' &&
            CCHAR(mvinch(2, right)) != '|' &&
            CCHAR(mvinch(2, left + 1)) != FLOOR)
            continue;
        for (x = left; x <= right; ++x)
        {
            char expected = chat(1, x);
            if ((expected == '-' || expected == DOOR) &&
                CCHAR(mvinch(1, x)) == ' ')
                jrogue_mvaddch(1, x, expected);
        }
    }
}

static int
read_legacy_save_header(int inf, int *slines, int *scols)
{
    if (encread(&sbuf.st_ino, sizeof(sbuf.st_ino), inf) != (int) sizeof(sbuf.st_ino))
        return FALSE;
    if (encread(&sbuf.st_dev, sizeof(sbuf.st_dev), inf) != (int) sizeof(sbuf.st_dev))
        return FALSE;
    if (encread(&sbuf.st_ctime, sizeof(sbuf.st_ctime), inf) != (int) sizeof(sbuf.st_ctime))
        return FALSE;
    if (encread(&sbuf.st_mtime, sizeof(sbuf.st_mtime), inf) != (int) sizeof(sbuf.st_mtime))
        return FALSE;
    if (encread(slines, sizeof(*slines), inf) != (int) sizeof(*slines))
        return FALSE;
    if (encread(scols, sizeof(*scols), inf) != (int) sizeof(*scols))
        return FALSE;

    return TRUE;
}

static int
read_save_header(int inf, int *slines, int *scols)
{
    int magic = 0;
    unsigned long saved_ino = 0, saved_dev = 0;
    long saved_ctime = 0, saved_mtime = 0;

    if (rs_read_int(inf, &magic) == FALSE)
        return FALSE;

    if (magic != SAVE_HEADER_MAGIC)
    {
        if (lseek(inf, -4, SEEK_CUR) == (off_t) -1)
            return FALSE;
        return read_legacy_save_header(inf, slines, scols);
    }

    if (rs_read_ulong(inf, &saved_ino) == FALSE)
        return FALSE;
    if (rs_read_ulong(inf, &saved_dev) == FALSE)
        return FALSE;
    if (rs_read_long(inf, &saved_ctime) == FALSE)
        return FALSE;
    if (rs_read_long(inf, &saved_mtime) == FALSE)
        return FALSE;
    if (rs_read_int(inf, slines) == FALSE)
        return FALSE;
    if (rs_read_int(inf, scols) == FALSE)
        return FALSE;

    (void) saved_ino;
    (void) saved_dev;
    (void) saved_ctime;
    (void) saved_mtime;

    return TRUE;
}

/*
 * save_game:
 *	Implement the "save game" command
 */
int save_game(void)
{
    register FILE *savef;
    register int c;
    char buf[PATHSTR];
    char prompt[PATHSTR + 128];

    /*
     * get file name
     */
    mpos = 0;
over:
    if (file_name[0] != '\0')
    {
	for (;;)
	{
	    format_save_confirmation(prompt, sizeof prompt, file_name, COLS);
	    msg("%s", prompt);
            jrogue_show_cursor(TRUE);
	    c = readchar();
            jrogue_restore_cursor();
	    mpos = 0;
	    if (c == ESCAPE)
	    {
		msg("");
		return FALSE;
	    }
	    else if (c == 'n' || c == 'N' || c == 'y' || c == 'Y')
		break;
	    else
		msg(JSTR("'Y'か'N'で答えてください。（ESCでキャンセル）", "please answer Y or N"));
	}
	if (c == 'y' || c == 'Y')
	{
	    snprintf(buf, sizeof buf, "%s", file_name);
	    goto gotfile;
	}
    }

    do
    {
	mpos = 0;
	msg(JSTR("ファイル名：", "file name: "));
	buf[0] = '\0';
	if (get_str_size(buf, stdscr, sizeof buf) == QUIT)
	{
quit:
	    msg("");
	    return FALSE;
	}
	mpos = 0;
gotfile:
	/*
	 * test to see if the file exists
	 */
	if (md_file_stat(buf, &sbuf) >= 0)
	{
	    for (;;)
	    {
		msg(JSTR("そのファイルは存在します。上書きしますか？", "File exists.  Do you wish to overwrite it?"));
		mpos = 0;
                jrogue_show_cursor(TRUE);
                c = readchar();
                jrogue_restore_cursor();
		if (c == ESCAPE)
		    goto quit;
		if (c == 'y' || c == 'Y')
		    break;
		else if (c == 'n' || c == 'N')
		    goto over;
		else
		    msg(JSTR("'Y'か'N'で答えてください。", "Please answer Y or N"));
	    }
	}
	snprintf(file_name, PATHSTR, "%s", buf);
	file_name[PATHSTR - 1] = '\0';
	utf8_trim_partial(file_name);
	if ((savef = md_save_fopen(file_name)) == NULL)
	{
	    if (jrogue_lang_is_japanese)
		msg("セーブファイルを作成できません：%s", file_name);
	    else
		msg("%s", strerror(errno));	/* fake perror() */
	}
    } while (savef == NULL);

    /*
     * write out encrpyted file (after a stat)
     * The fwrite is to force allocation of the buffer before the write
     */
    /* The confirmation may have been replaced by an overwrite question.
     * Show the final filename once more, then retain it very briefly so it
     * was readable on a real terminal.  save_file() subsequently clears the
     * message row and serializes the game, as before. */
    format_save_path_message(prompt, sizeof prompt,
                             JSTR("ファイル名： ", "file name: "),
                             file_name, "", COLS);
    mpos = 0;
    msg("%s", prompt);
    md_delay(SAVE_FILENAME_DELAY_MS);

    save_file(savef);
    return TRUE;
}

/*
 * auto_save:
 *	Automatically save a file.  This is used if a HUP signal is
 *	recieved
 */
void auto_save(int sig)
{
    register FILE *savef;

    (void)sig;

    md_ignore_signals();

    if (file_name[0] != '\0' && (savef = md_save_fopen(file_name)) != NULL)
	save_file(savef);
    rogue_endwin();
    exit(1);
}

/*
 * save_file:
 *	Write the saved game on the file
 */
void save_file(FILE * savef)
{
    int slines = LINES;
    int scols  = COLS;

    /*
     * close any open score file
     */
    close(fd);
    move(LINES-1, 0);
    refresh();
    fstat(md_fileno(savef), &sbuf);
    /*
     * DO NOT DELETE.  This forces stdio to allocate the output buffer
     * so that malloc doesn't get confused on restart
     */
    fwrite("junk", 1, 5, savef);

    fseek(savef, 0L, 0);

    encwrite(version,strlen(version)+1,savef);
    rs_write_int(savef, SAVE_HEADER_MAGIC);
    rs_write_ulong(savef, (unsigned long) sbuf.st_ino);
    rs_write_ulong(savef, (unsigned long) sbuf.st_dev);
    rs_write_long(savef, (long) sbuf.st_ctime);
    rs_write_long(savef, (long) sbuf.st_mtime);
    rs_write_int(savef, slines);
    rs_write_int(savef, scols);
    msg("");
    rs_save_file(savef);

    fclose(savef);
}

/* On any failed restore, release the file descriptor and restore the
 * terminal before printing the reason.  Windows must leave the saved game
 * untouched on failure; successful restores retain the one-use behavior.
 * On Unix the original early unlink remains unchanged.
 */
static void
restore_abort(int inf, int screen_started)
{
    if (screen_started)
    {
        jrogue_show_cursor(TRUE);
        rogue_endwin();
    }
    if (inf >= 0)
        (void)close(inf);
}

/*
 * restore:
 *	Restore a saved game from a file with elaborate checks for file
 *	integrity from cheaters
 */
int restore(char * file, char * * envp)
{
    register int inf;
    register bool syml;
#ifndef _WIN32
    extern char **environ;
#else
    /* UCRT manages environ itself; assigning to the imported CRT symbol
     * through a redeclaration is neither needed nor portable. */
    (void)envp;
#endif
    char buf[PATHSTR];
    char rmsg[PATHSTR + 80];
    STAT sbuf2;
    int slines, scols;

    if (strcmp(file, "-r") == 0)
	file = file_name;

#ifdef SIGTSTP
    /*
     * If a process can be suspended, this code wouldn't work
     */
    signal(SIGTSTP, SIG_IGN);
#endif

    if ((inf = md_file_open(file, O_RDONLY, 0)) < 0)
    {
	if (jrogue_lang_is_japanese)
	    printf("セーブファイル %s を開けません\n", file);
	else
	    perror(file);
	return FALSE;
    }

    fflush(stdout);
    if (encread(buf, strlen(version) + 1, inf) != (int) strlen(version) + 1 ||
        strcmp(buf, version) != 0)
    {
        restore_abort(inf, FALSE);
        printf("%s", JSTR("古いバージョンのセーブファイルは使えません。\n", "Sorry, saved game is out of date.\n"));
        return FALSE;
    }

    if (fstat(inf, &sbuf2) < 0)
    {
        restore_abort(inf, FALSE);
        printf("%s", JSTR("セーブファイルの情報を取得できません。\n",
                         "Cannot inspect save file.\n"));
        return FALSE;
    }
    fflush(stdout);
    syml = issymlink(file);
#ifndef _WIN32
    /* Unix permits unlinking an open file, which also prevents a second
     * restore while the first one is still reading its saved state. */
    if (
#ifdef WIZARD
        !wizard &&
#endif
        md_unlink(file) < 0)
    {
        restore_abort(inf, FALSE);
        printf("%s", JSTR("ファイルを削除できません。\n", "Cannot unlink file\n"));
        return FALSE;
    }
#endif

    fflush(stdout);

    if (read_save_header(inf, &slines, &scols) == FALSE)
    {
        restore_abort(inf, FALSE);
        printf("%s", JSTR("セーブファイルのヘッダを読み込めません。\n", "Cannot read save file header.\n"));
        return FALSE;
    }

    /*
     * we do not close the file so that we will have a hold of the
     * inode for as long as possible
     */

    if (initscr() == NULL)
    {
        restore_abort(inf, FALSE);
        printf("%s", JSTR("端末画面を初期化できません。\n",
                         "Cannot initialize terminal screen.\n"));
        return FALSE;
    }
#ifdef _WIN32
    jrogue_windows_retry_direct_launch_resize();
#endif
    jrogue_color_init();
    jrogue_restore_cursor();

    /* A damaged save header must not create zero- or negative-sized
     * windows, even if its recorded dimensions fit the current terminal. */
    if (slines <= 0 || scols <= 0)
    {
        restore_abort(inf, TRUE);
        printf("%s", JSTR("セーブファイルの画面サイズが不正です。\n",
                         "Invalid screen dimensions in saved game.\n"));
        return FALSE;
    }

    if (slines > LINES) 
    { 
        int current_lines = LINES;
        restore_abort(inf, TRUE);
        printf(JSTR("セーブ元のゲームは%d行でプレイされていたようです。\n", "Sorry, original game was played on a screen with %d lines.\n"), slines); 
        printf(JSTR("このスクリーンは%d行なので、そのファイルを使うことはできません。\n", "Current screen only has %d lines. Unable to restore game\n"), current_lines); 
        return(FALSE); 
    } 
    
    if (scols > COLS) 
    { 
        int current_cols = COLS;
        restore_abort(inf, TRUE);
        printf(JSTR("セーブ元のゲームは%d列でプレイされていたようです。\n", "Sorry, original game was played on a screen with %d columns.\n"), scols); 
        printf(JSTR("このスクリーンは%d列なので、そのファイルを使うことはできません。\n", "Current screen only has %d columns. Unable to restore game\n"), current_cols); 
        return(FALSE); 
    }

    hw = newwin(LINES, COLS, 0, 0);
    if (hw == NULL)
    {
        restore_abort(inf, TRUE);
        printf("%s", JSTR("画面を作成できません。\n",
                         "Cannot create restore window.\n"));
        return FALSE;
    }
    jrogue_color_apply_window(hw);
    keypad(stdscr,1);
    
    mpos = 0;
    /* Do not print over the screen before its saved cells are restored.
     * The filename and timestamp are shown safely on row 0 afterwards. */

    /*
     * defeat multiple restarting from the same place
     */
#ifdef WIZARD
    if (!wizard)
#endif
	if (sbuf2.st_nlink != 1 || syml)
	{
            restore_abort(inf, TRUE);
	    printf("%s", JSTR("そのファイルから再開することはできません。\n", "Cannot restore from a linked file\n"));
	    return FALSE;
	}

    if (rs_restore_file(inf) == FALSE)
    {
        restore_abort(inf, TRUE);
        printf("%s", JSTR("セーブファイルを復元できません。\n", "Cannot restore file\n"));
        return(FALSE);
    }

#ifdef _WIN32
    /* Windows does not allow _wunlink() while the save is open with
     * _wopen()'s default sharing mode.  Preserve the save if reading
     * failed, and only consume it after the entire restore succeeded.
     * md_unlink_open_file closes inf before attempting the Unicode unlink.
     * Apply the original one-use rule to ordinary games, not wizard mode. */
#ifdef WIZARD
    if (wizard)
        close(inf);
    else
#endif
    if (md_unlink_open_file(file, inf) < 0)
    {
        restore_abort(-1, TRUE); /* md_unlink_open_file already closed inf. */
        printf("%s", JSTR("セーブファイルを削除できません。\n",
                          "Cannot remove restored save file.\n"));
        return FALSE;
    }
#endif

    repair_saved_top_room_walls();
    jrogue_color_refresh();

#ifdef SIGTSTP
    signal(SIGTSTP, tstp);
#endif
#ifndef _WIN32
    environ = envp;
#endif
    snprintf(file_name, PATHSTR, "%s", file);
    file_name[PATHSTR - 1] = '\0';
    utf8_trim_partial(file_name);
    setup();
    jrogue_restore_cursor();
    clearok(curscr, TRUE);
    touchwin(stdscr);
    srand(getpid());
    format_restore_message(rmsg, sizeof rmsg, file, sbuf2.st_mtime, COLS);
    msg("%s", rmsg);
	status();
    playit();
    return 0;
}

/*
 * encwrite:
 *	Perform an encrypted write
 */
void encwrite(void * starta, unsigned int size, FILE * outf)
{
    register char *ep;
    register char *start = (char *) starta;
    ep = encstr;

    while (size--)
    {
	putc(*start++ ^ *ep++, outf);
	if (*ep == '\0')
	    ep = encstr;
    }
}

/*
 * encread:
 *	Perform an encrypted read
 */
int encread(void * starta, unsigned int size, int inf)
{
    register char *ep;
    register int read_size;
    register char *start = (char *) starta;
    register int left;

    if ((read_size = read(inf, start, size)) == -1 || read_size == 0)
        return read_size;

    ep = encstr;
    left = read_size;

    while (left--)
    {
        *start++ ^= *ep++;
        if (*ep == '\0')
            ep = encstr;
    }

    return read_size;
}
