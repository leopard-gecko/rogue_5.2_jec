/* Rogue 5.2 JEC: Windows-only PDCursesMod/Unicode services.
 * Keep Win32 headers out of the portable game's sources. */
#ifndef ROGUE52_WINDOWS_H
#define ROGUE52_WINDOWS_H
#ifdef _WIN32
#include <stdio.h>
#include <sys/stat.h>
#include <wchar.h>
void jrogue_windows_console_init(void);  /* before the first initscr() */
/* Windows VT cursor-style restoration and redraw handling. */
void jrogue_windows_restore_cursor_shape(void);
void jrogue_windows_cursor_invalidate(void);
void jrogue_windows_show_cursor(int show);
int jrogue_windows_refresh(WINDOW *win);
void jrogue_windows_init_palette(void);   /* after start_color() */
char *jrogue_windows_rogueopts_utf8(void); /* allocated; caller frees */
char *jrogue_windows_username_utf8(void); /* allocated; caller frees */
char *jrogue_windows_documents_utf8(void);/* allocated; caller frees */
int jrogue_windows_score_vt(void);
/* Clear the current console viewport after endwin(), before plain scores. */
void jrogue_windows_clear_plain_screen(void);
/* Native OS record lock on the first byte of the open score file. */
int jrogue_windows_score_lock(int score_fd);
void jrogue_windows_score_unlock(int score_fd);
/* Only a directly opened console pauses after showing the leaderboard. */
void jrogue_windows_wait_for_score_exit(void);
/* Resize only a direct Explorer launch, never an existing shell session. */
void jrogue_windows_resize_direct_launch(void);
void jrogue_windows_retry_direct_launch_resize(void);
int jrogue_windows_is_direct_launch(void);
/* Keep direct-launch restore errors readable after the console reports them. */
void jrogue_windows_wait_for_restore_error(void);
/* Command line / saved-game chooser use Windows UTF-16. Returned strings
 * are malloc-owned UTF-8; the caller frees them. */
char *jrogue_windows_argument_utf8(int index);
char *jrogue_windows_launch_save(const char *default_save);
/* Wide CRT file APIs preserve Japanese/emoji paths. */
FILE *jrogue_windows_fopen_utf8(const char *path, const wchar_t *mode);
int jrogue_windows_open_utf8(const char *path, int flags, int mode);
int jrogue_windows_stat_utf8(const char *path, struct stat *result);
int jrogue_windows_unlink_utf8(const char *path);
int jrogue_windows_chmod_utf8(const char *path, int mode);
/* Option editor receives complete Unicode scalars instead of mojibake. */
int jrogue_windows_read_option_key(int *result);
/* Render non-BMP UTF-8 using a single wide curses cell, then correct
 * the PDCursesMod VT output after option-window refreshes. */
int jrogue_windows_waddstr_utf8(WINDOW *win, const char *text);
void jrogue_windows_option_vt_restore_nonbmp(WINDOW *win);
#endif
#endif
