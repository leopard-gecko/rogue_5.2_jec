/* Windows console, Unicode file/keyboard and direct-launch services.
 * VT-only approach follows the working Rogue 5.4.5 JEC Windows port.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <commdlg.h>
#include <tlhelp32.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
/* PDCurses also defines MOUSE_MOVED. */
#undef MOUSE_MOVED
#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "rogue.h"
#undef wrefresh
#undef refresh
#include "rogue_windows.h"
#include "jec/language.h"
#include "jec/unicode_text.h"

static char *
wide_to_utf8(const wchar_t *src)
{
    int need;
    char *out;

    if (!src)
        return NULL;
    need = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                src, -1, NULL, 0, NULL, NULL);
    if (need <= 0)
        return NULL;
    out = (char *)malloc((size_t)need);
    if (!out)
        return NULL;
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                            src, -1, out, need, NULL, NULL) != need)
    {
        free(out);
        return NULL;
    }
    return out;
}

static UINT saved_input_cp, saved_output_cp;
static int restore_codepages;

static void
jrogue_windows_restore_codepages(void)
{
    if (!restore_codepages)
        return;
    if (saved_input_cp)
        (void)SetConsoleCP(saved_input_cp);
    if (saved_output_cp)
        (void)SetConsoleOutputCP(saved_output_cp);
    restore_codepages = 0;
}

void
jrogue_windows_console_init(void)
{
    const char *vt = getenv("PDC_VT");
    if (!vt || !*vt)
        (void)_putenv("PDC_VT=RGB");
    else if (strstr(vt, "RGB") == NULL)
    {
        size_t need = strlen(vt) + sizeof "PDC_VT= RGB";
        char *value = (char *)malloc(need);
        if (value)
        {
            snprintf(value, need, "PDC_VT=%s RGB", vt);
            (void)_putenv(value);
            free(value);
        }
    }
    saved_input_cp = GetConsoleCP();
    saved_output_cp = GetConsoleOutputCP();
    if (!restore_codepages && (saved_input_cp || saved_output_cp))
    {
        restore_codepages = 1;
        (void)atexit(jrogue_windows_restore_codepages);
    }
    (void)SetConsoleOutputCP(CP_UTF8);
    (void)SetConsoleCP(CP_UTF8);
}


/* PDCursesMod VT calls PDC_curs_set(2) when endwin() closes the screen,
 * and curs_set(1) selects an underline during ordinary gameplay.
 * DECSCUSR 0 restores the terminal profile's default cursor shape.
 * Restore it AFTER curses has emitted its own shape change. */
static void
jrogue_windows_cursor_vt(const char *sequence, size_t length)
{
    const DWORD vt_flags = ENABLE_PROCESSED_OUTPUT |
                           ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode, written;
    int changed_mode = FALSE;

    if (output == NULL || output == INVALID_HANDLE_VALUE)
        return;

    if (GetConsoleMode(output, &mode))
    {
        if ((mode & vt_flags) != vt_flags)
        {
            if (!SetConsoleMode(output, mode | vt_flags))
                return;
            changed_mode = TRUE;
        }
        /* Preserve the order of CRT/PDCurses output and Win32 VT writes. */
        (void)fflush(stdout);
        (void)WriteFile(output, sequence, (DWORD)length, &written, NULL);
        if (changed_mode)
            (void)SetConsoleMode(output, mode);
    }
    else
    {
        /* mintty and similar ANSI PTYs can expose stdout as a pipe.
         * Never send cursor controls into a redirected regular file. */
        const char *term = getenv("TERM");
        if (GetFileType(output) == FILE_TYPE_PIPE && term != NULL &&
            (strncmp(term, "xterm", 5) == 0 ||
             strncmp(term, "screen", 6) == 0 ||
             strncmp(term, "tmux", 4) == 0 ||
             strncmp(term, "rxvt", 4) == 0 ||
             strncmp(term, "cygwin", 6) == 0))
        {
            (void)fwrite(sequence, 1, length, stdout);
            (void)fflush(stdout);
        }
    }
}

void
jrogue_windows_restore_cursor_shape(void)
{
    static const char reset[] = "\033[0 q";
    jrogue_windows_cursor_vt(reset, sizeof reset - 1);
}

/* Cache the current requested visibility. The game uses this rather than
 * show_cursor (the option's preference) while input dialogs temporarily show
 * the cursor. The cache is invalidated whenever curses exits screen mode. */
static int jrogue_windows_cursor_visible = -1;
static int jrogue_windows_first_refresh_pending = TRUE;

void
jrogue_windows_cursor_invalidate(void)
{
    jrogue_windows_cursor_visible = -1;
    jrogue_windows_first_refresh_pending = TRUE;
}

void
jrogue_windows_show_cursor(int show)
{
    show = show ? TRUE : FALSE;
    if (jrogue_windows_cursor_visible != show)
    {
        (void)curs_set(show ? 1 : 0);
        jrogue_windows_cursor_visible = show;
        /* PDCursesMod assigns an underline for curs_set(1); override it
         * only when the visibility changes, not on every game turn. */
        if (show)
            jrogue_windows_restore_cursor_shape();
    }
}

int
jrogue_windows_refresh(WINDOW *win)
{
    static const char hide[] = "\033[?25l";
    static const char show_default[] = "\033[?25h\033[0 q";
    int returning_from_endwin = isendwin();
    int visible = jrogue_windows_cursor_visible;
    int result;

    if (visible < 0)
        visible = show_cursor ? TRUE : FALSE;

    /* A VT renderer moves the physical cursor around while painting. Hide
     * only the *physical* cursor: do not disturb PDCurses' logical cursor
     * position or visibility via curs_set() during a refresh. */
    if (visible || returning_from_endwin)
        jrogue_windows_cursor_vt(hide, sizeof hide - 1);

    result = wrefresh(win); /* Actual PDCurses call (this file undefines the wrapper). */

    /* 5.4.5 JEC startup fix: after the first paint, allow any deferred
     * palette redraw scheduled by init_color() to complete. Do this before
     * supplementary UTF-8 painting so late palette updates cannot erase it. */
    if (result != ERR && jrogue_windows_first_refresh_pending)
    {
        (void)napms(0);
        jrogue_windows_first_refresh_pending = FALSE;
    }

    /* In the option/inventory window, repaint UTF-8 supplementary scalars
     * before making the cursor visible again. This also avoids a transient
     * cursor jump while the PDCursesMod VT glyph workaround is running. */
    if (result != ERR && win == hw)
    {
        (void)fflush(stdout);
        jrogue_windows_option_vt_restore_nonbmp(win);
    }

    if (returning_from_endwin && result != ERR)
    {
        /* reset_prog_mode() may have overwritten visibility and shape as
         * part of wrefresh(); apply our option after that internal reset. */
        jrogue_windows_cursor_visible = -1;
        jrogue_windows_show_cursor(visible);
    }

    /* Restore physical visibility even when painting fails, so a failed
     * wrefresh() cannot leave the terminal cursor permanently hidden. */
    if (visible)
        jrogue_windows_cursor_vt(show_default, sizeof show_default - 1);
    else
        jrogue_windows_cursor_vt(hide, sizeof hide - 1);
    return result;
}

char *
jrogue_windows_rogueopts_utf8(void)
{
    DWORD count = GetEnvironmentVariableW(L"ROGUEOPTS", NULL, 0);
    DWORD got;
    wchar_t *wide;
    char *result;
    if (!count || count > 32767)
        return NULL;
    wide = (wchar_t *)malloc((size_t)count * sizeof *wide);
    if (!wide)
        return NULL;
    got = GetEnvironmentVariableW(L"ROGUEOPTS", wide, count);
    result = (got && got < count) ? wide_to_utf8(wide) : NULL;
    free(wide);
    return result;
}

char *
jrogue_windows_username_utf8(void)
{
    DWORD count = 0;
    wchar_t *name;
    char *result;
    (void)GetUserNameW(NULL, &count);
    if (!count || count > 32767)
        return NULL;
    name = (wchar_t *)malloc((size_t)count * sizeof *name);
    if (!name)
        return NULL;
    result = GetUserNameW(name, &count) ? wide_to_utf8(name) : NULL;
    free(name);
    return result;
}

char *
jrogue_windows_documents_utf8(void)
{
    wchar_t path[MAX_PATH];
    /* CSIDL_PERSONAL is the Documents folder also used by the old port. */
    if (SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT,
                          path) != S_OK)
        return NULL;
    return wide_to_utf8(path);
}

int
jrogue_windows_score_vt(void)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    if (out == NULL || out == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(out, &mode))
        return 0;
    if (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)
        return 1;
    return SetConsoleMode(out, mode | ENABLE_PROCESSED_OUTPUT |
                                   ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
}

/* Clear the plain score/exit screen after curses has called endwin().
 * The game prints scores with stdio, not curses.  The Unix clear sequence
 * works in Windows Terminal/modern Console Host when VT processing is
 * enabled; the native API covers legacy Console Host without VT support.
 * Do nothing for redirection or other non-console output handles.
 */
void
jrogue_windows_clear_plain_screen(void)
{
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD origin;
    DWORD row, width, written;
    static const char clear_vt[] = "\033[0m\033[H\033[2J";

    if (output == NULL || output == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(output, &mode))
        return;

    /* Never let buffered text from an earlier menu appear after clearing. */
    (void)fflush(stdout);
    if (jrogue_windows_score_vt() &&
        WriteFile(output, clear_vt, (DWORD)(sizeof clear_vt - 1),
                  &written, NULL) &&
        written == (DWORD)(sizeof clear_vt - 1))
        return;

    if (!GetConsoleScreenBufferInfo(output, &info))
        return;
    origin.X = info.srWindow.Left;
    origin.Y = info.srWindow.Top;
    width = (DWORD)(info.srWindow.Right - info.srWindow.Left + 1);
    /* Clear only the visible viewport, not the entire scrollback buffer. */
    for (row = (DWORD)info.srWindow.Top;
         row <= (DWORD)info.srWindow.Bottom; ++row)
    {
        COORD start;
        start.X = origin.X;
        start.Y = (SHORT)row;
        (void)FillConsoleOutputCharacterW(output, L' ', width,
                                          start, &written);
        (void)FillConsoleOutputAttribute(output, info.wAttributes,
                                         width, start, &written);
    }
    (void)SetConsoleCursorPosition(output, origin);
}

/* A Windows kernel lock, held on the same handle as the score data.
 * The OVERLAPPED structure must remain alive until UnlockFileEx. */
static OVERLAPPED score_lock_offset;
static int score_lock_active;

int
jrogue_windows_score_lock(int score_fd)
{
    intptr_t handle_value = _get_osfhandle(score_fd);
    HANDLE handle;
    DWORD code;

    if (handle_value == -1) { errno = EBADF; return 0; }
    handle = (HANDLE)handle_value;
    if (!LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK |
                            LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0,
                    &score_lock_offset))
    {
        code = GetLastError();
        errno = (code == ERROR_LOCK_VIOLATION) ? EACCES : EIO;
        return 0;
    }
    score_lock_active = 1;
    return 1;
}

void
jrogue_windows_score_unlock(int score_fd)
{
    intptr_t handle_value = _get_osfhandle(score_fd);
    if (!score_lock_active) return;
    if (handle_value != -1)
        (void)UnlockFileEx((HANDLE)handle_value, 0, 1, 0,
                           &score_lock_offset);
    score_lock_active = 0;
}

void
jrogue_windows_wait_for_score_exit(void)
{
    wint_t ch;
    if (!jrogue_windows_is_direct_launch() ||
        !_isatty(_fileno(stdout)))
        return;
    fputs(JSTR("\r\n［リターンを押すと終了します］",
              "\r\n[Press Enter to exit]"), stdout);
    fflush(stdout);
    for (;;)
    {
        ch = _getwch();
        if (ch == '\r' || ch == '\n' || ch == WEOF)
            break;
        if (ch == 0 || ch == 0xe0)
            (void)_getwch();
    }
    fputs("\r\n", stdout);
}

/* PDCursesMod VT interprets RGB-accurate color values when PDC_VT=RGB.
 * Set the standard xterm 256-color RGB palette explicitly so the UNIX
 * symbol palette and the dedicated 5.2 monster RGB slots match.
 * Custom monster slots 64..89 are overwritten by the main color module.
 */
void
jrogue_windows_init_palette(void)
{
    static const unsigned char ansi[16][3] = {
        {0,0,0},{128,0,0},{0,128,0},{128,128,0},
        {0,0,128},{128,0,128},{0,128,128},{192,192,192},
        {128,128,128},{255,0,0},{0,255,0},{255,255,0},
        {0,0,255},{255,0,255},{0,255,255},{255,255,255}
    };
    static const int steps[6] = {0,95,135,175,215,255};
    static const short base[16] = {
        COLOR_BLACK, COLOR_RED, COLOR_GREEN, COLOR_YELLOW,
        COLOR_BLUE, COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE,
        COLOR_BLACK+8,COLOR_RED+8,COLOR_GREEN+8,COLOR_YELLOW+8,
        COLOR_BLUE+8,COLOR_MAGENTA+8,COLOR_CYAN+8,COLOR_WHITE+8
    };
    int ix, r, g, b, n, count;
    short dst;
    if (!can_change_color())
        return;
    count = COLORS < 256 ? COLORS : 256;
    for (ix = 0; ix < count; ++ix)
    {
        if (ix < 16)
        {
            r = ansi[ix][0]; g = ansi[ix][1]; b = ansi[ix][2];
            dst = base[ix];
        }
        else if (ix < 232)
        {
            n = ix - 16;
            r = steps[n/36]; g = steps[(n/6)%6]; b = steps[n%6];
            dst = (short)ix;
        }
        else
        {
            r = g = b = 8 + (ix-232)*10;
            dst = (short)ix;
        }
        (void)init_color(dst, (short)((r*1000+127)/255),
                              (short)((g*1000+127)/255),
                              (short)((b*1000+127)/255));
    }
}

/* UTF-8 paths never pass through UCRT's active ANSI codepage. */
static wchar_t *
utf8_to_wide(const char *src)
{
    int chars;
    wchar_t *result;

    if (!src) { errno = EINVAL; return NULL; }
    chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                src, -1, NULL, 0);
    if (chars <= 0) { errno = EINVAL; return NULL; }
    result = (wchar_t *)malloc((size_t)chars * sizeof *result);
    if (!result) return NULL;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                            src, -1, result, chars) != chars)
    {
        free(result);
        errno = EINVAL;
        return NULL;
    }
    return result;
}

FILE *
jrogue_windows_fopen_utf8(const char *path, const wchar_t *mode)
{
    FILE *fp;
    wchar_t *wide = utf8_to_wide(path);
    if (!wide) return NULL;
    fp = _wfopen(wide, mode);
    free(wide);
    return fp;
}

int
jrogue_windows_open_utf8(const char *path, int flags, int mode)
{
    int result;
    wchar_t *wide = utf8_to_wide(path);
    if (!wide) return -1;
    result = _wopen(wide, flags | _O_BINARY, mode);
    free(wide);
    return result;
}

int
jrogue_windows_stat_utf8(const char *path, struct stat *result)
{
    struct _stat64 wide_stat;
    wchar_t *wide = utf8_to_wide(path);
    int rc;
    if (!wide) return -1;
    rc = _wstat64(wide, &wide_stat);
    free(wide);
    if (rc == 0 && result)
    {
        /* Copy only portable fields used by Rogue.  A Windows stat's
         * inode and device are not used to identify saved files. */
        memset(result, 0, sizeof *result);
        result->st_mode  = wide_stat.st_mode;
        result->st_nlink = wide_stat.st_nlink;
        result->st_size  = wide_stat.st_size;
        result->st_mtime = wide_stat.st_mtime;
        result->st_ctime = wide_stat.st_ctime;
        result->st_ino   = wide_stat.st_ino;
        result->st_dev   = wide_stat.st_dev;
    }
    return rc;
}

int
jrogue_windows_unlink_utf8(const char *path)
{
    int result;
    wchar_t *wide = utf8_to_wide(path);
    if (!wide) return -1;
    result = _wunlink(wide);
    free(wide);
    return result;
}

int
jrogue_windows_chmod_utf8(const char *path, int mode)
{
    int result;
    wchar_t *wide = utf8_to_wide(path);
    if (!wide) return -1;
    result = _wchmod(wide, mode);
    free(wide);
    return result;
}

char *
jrogue_windows_argument_utf8(int index)
{
    int count = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &count);
    char *result = NULL;
    if (argv)
    {
        if (index > 0 && index < count)
            result = wide_to_utf8(argv[index]);
        LocalFree(argv);
    }
    return result;
}

/* Windows Terminal/ConPTY needs CSI 8; Console Host also needs the
 * Win32 size APIs.  Never resize a shell-owned/shared console. */
#define ROGUE_WINDOW_COLS 80
#define ROGUE_WINDOW_ROWS 24
static int resize_permission;

static int
is_shell(const wchar_t *name)
{
    static const wchar_t *const shells[] = {
        L"cmd.exe", L"powershell.exe", L"pwsh.exe", L"bash.exe",
        L"sh.exe", L"mintty.exe", L"wsl.exe", L"zsh.exe", L"fish.exe"
    };
    size_t i;
    for (i = 0; i < sizeof shells / sizeof shells[0]; ++i)
        if (lstrcmpiW(name, shells[i]) == 0) return 1;
    return 0;
}

static int
parent_launch_kind(void)
{
    HANDLE snapshot;
    PROCESSENTRY32W entry;
    DWORD parent = 0, self = GetCurrentProcessId();
    int result = 0;
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    entry.dwSize = sizeof entry;
    if (Process32FirstW(snapshot, &entry))
    {
        do { if (entry.th32ProcessID == self) {
            parent = entry.th32ParentProcessID; break; }
        } while (Process32NextW(snapshot, &entry));
    }
    if (parent && Process32FirstW(snapshot, &entry))
    {
        do {
            if (entry.th32ProcessID != parent) continue;
            if (is_shell(entry.szExeFile)) result = -1;
            else if (lstrcmpiW(entry.szExeFile, L"explorer.exe") == 0 ||
                     lstrcmpiW(entry.szExeFile, L"WindowsTerminal.exe") == 0 ||
                     lstrcmpiW(entry.szExeFile, L"WindowsTerminalPreview.exe") == 0 ||
                     lstrcmpiW(entry.szExeFile, L"wt.exe") == 0 ||
                     lstrcmpiW(entry.szExeFile, L"OpenConsole.exe") == 0 ||
                     lstrcmpiW(entry.szExeFile, L"conhost.exe") == 0)
                result = 1;
            break;
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return result;
}

static int
console_has_shell(int *sole)
{
    DWORD pids[64], count, self = GetCurrentProcessId();
    HANDLE snapshot;
    PROCESSENTRY32W entry;
    DWORD i;
    int found = 0;
    count = GetConsoleProcessList(pids, 64);
    if (count == 0 || count > 64) return -1;
    *sole = (count == 1 && pids[0] == self);
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return -1;
    entry.dwSize = sizeof entry;
    if (Process32FirstW(snapshot, &entry))
    {
        do {
            if (!is_shell(entry.szExeFile)) continue;
            for (i = 0; i < count; ++i)
                if (pids[i] == entry.th32ProcessID) { found = 1; break; }
            if (found) break;
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return found;
}

int
jrogue_windows_is_direct_launch(void)
{
    return resize_permission == 1;
}

static void
resize_classic_console(HANDLE out)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD target, maximum;
    SMALL_RECT rect;
    SHORT w, h;
    if (!GetConsoleScreenBufferInfo(out, &info)) return;
    maximum = GetLargestConsoleWindowSize(out);
    if (maximum.X < ROGUE_WINDOW_COLS || maximum.Y < ROGUE_WINDOW_ROWS)
        return;
    w = (SHORT)(info.srWindow.Right - info.srWindow.Left + 1);
    h = (SHORT)(info.srWindow.Bottom - info.srWindow.Top + 1);
    rect.Left = 0; rect.Top = 0;
    rect.Right = (w < ROGUE_WINDOW_COLS ? w : ROGUE_WINDOW_COLS) - 1;
    rect.Bottom = (h < ROGUE_WINDOW_ROWS ? h : ROGUE_WINDOW_ROWS) - 1;
    if (!SetConsoleWindowInfo(out, TRUE, &rect)) return;
    target.X = ROGUE_WINDOW_COLS;
    target.Y = ROGUE_WINDOW_ROWS;
    if (!SetConsoleScreenBufferSize(out, target)) return;
    rect.Right = ROGUE_WINDOW_COLS - 1;
    rect.Bottom = ROGUE_WINDOW_ROWS - 1;
    (void)SetConsoleWindowInfo(out, TRUE, &rect);
}

static void
resize_direct_console(void)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode, written;
    int parent, only_client = 0, shell;
    HWND window;
    static const char request[] = "\033[8;24;80t";
    if (resize_permission != 0 || out == INVALID_HANDLE_VALUE || !out ||
        !GetConsoleMode(out, &mode)) return;
    parent = parent_launch_kind();
    if (parent == -1) { resize_permission = -1; return; }
    shell = console_has_shell(&only_client);
    if (shell > 0) { resize_permission = -1; return; }
    if (shell < 0 || (parent == 0 && !only_client)) return;
    resize_permission = 1;
    window = GetConsoleWindow();
    if (window && IsWindowVisible(window) && only_client)
        resize_classic_console(out);
    if (!(mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING))
        if (!SetConsoleMode(out, mode | ENABLE_PROCESSED_OUTPUT |
                                ENABLE_VIRTUAL_TERMINAL_PROCESSING))
            return;
    (void)WriteFile(out, request, (DWORD)(sizeof request - 1),
                    &written, NULL);
}

void jrogue_windows_resize_direct_launch(void) { resize_direct_console(); }
void jrogue_windows_retry_direct_launch_resize(void) { resize_direct_console(); }

static int
file_exists_utf8(const char *path)
{
    wchar_t *wide = utf8_to_wide(path);
    DWORD kind;
    if (!wide) return 0;
    kind = GetFileAttributesW(wide);
    free(wide);
    return kind != INVALID_FILE_ATTRIBUTES && !(kind & FILE_ATTRIBUTE_DIRECTORY);
}

/* A direct Explorer launch otherwise closes before users can read why
 * their save could not be restored.  Shell launches remain non-blocking. */
void
jrogue_windows_wait_for_restore_error(void)
{
    wint_t c;
    if (!jrogue_windows_is_direct_launch())
        return;
    fputs("\n", stderr);
    fputs(JSTR("［リターンを押すと終了します］",
               "[Press Enter to exit]"), stderr);
    fflush(stderr);
    for (;;)
    {
        c = _getwch();
        if (c == '\r' || c == '\n' || c == WEOF)
            break;
        if (c == 0 || c == 0xe0)
            (void)_getwch();
    }
    fputc('\n', stderr);
}

static char *
choose_save_file(void)
{
    static const wchar_t filters[] =
        L"Saved games (*.save)\0*.save\0All files (*.*)\0*.*\0";
    OPENFILENAMEW dialog;
    wchar_t *buf = (wchar_t *)calloc(32768, sizeof(wchar_t));
    char *result = NULL;
    if (!buf) return NULL;
    memset(&dialog, 0, sizeof dialog);
    dialog.lStructSize = sizeof dialog;
    dialog.lpstrFilter = filters;
    dialog.lpstrFile = buf;
    dialog.nMaxFile = 32768;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    dialog.lpstrTitle = L"Choose saved game / セーブファイルを選ぶ";
    if (GetOpenFileNameW(&dialog)) result = wide_to_utf8(buf);
    free(buf);
    return result;
}

char *
jrogue_windows_launch_save(const char *default_save)
{
    int key;
    char *selected;
    if (!jrogue_windows_is_direct_launch() ||
        !file_exists_utf8(default_save)) return NULL;
    for (;;)
    {
        puts("\n                   Rogue 5.2\n");
        puts(JSTR("セーブした冒険が見つかりました。",
                  "A saved adventure was found."));
        printf("[R/Enter]  %s\n", JSTR("セーブした冒険を再開する",
                                    "Resume saved adventure"));
        printf("[N]        %s\n", JSTR("新しい冒険を始める",
                                    "Start a new adventure"));
        printf("[F]        %s\n", JSTR("別のセーブファイルを選ぶ",
                                    "Choose another save file"));
        printf("[Q]        %s\n", JSTR("終了する", "Quit"));
        fputs(JSTR("キーを押してください: ", "Press a key: "), stdout);
        fflush(stdout);
        key = _getwch();
        if (key == 0 || key == 0xe0) { (void)_getwch(); continue; }
        putchar('\n');
        if (key == 'r' || key == 'R' || key == '\r' || key == '\n')
            return _strdup(default_save);
        if (key == 'n' || key == 'N') return NULL;
        if (key == 'q' || key == 'Q' || key == 27) exit(0);
        if (key == 'f' || key == 'F')
        {
            selected = choose_save_file();
            if (selected) return selected;
        }
    }
}

/* Input events are UTF-16, including IME text.  Combine surrogate pairs
 * before handing a codepoint to the UTF-8 option editor. */
int
jrogue_windows_read_option_key(int *result)
{
    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode, count;
    INPUT_RECORD record;
    static WCHAR down_high, down_low;
    unsigned int high = 0;

    if (!result || input == NULL || input == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(input, &mode))
        return 0;
    for (;;)
    {
        KEY_EVENT_RECORD *key;
        unsigned int cp;
        int keydown;

        if (!ReadConsoleInputW(input, &record, 1, &count) || count != 1)
        {
            *result = ERR;
            return 1;
        }
        if (record.EventType != KEY_EVENT)
            continue;
        key = &record.Event.KeyEvent;
        keydown = (key->bKeyDown != FALSE);
        cp = (unsigned int)key->uChar.UnicodeChar;

        if (keydown)
        {
            switch (key->wVirtualKeyCode)
            {
                case VK_LEFT: *result = KEY_LEFT; return 1;
                case VK_RIGHT: *result = KEY_RIGHT; return 1;
                case VK_HOME: *result = KEY_HOME; return 1;
                case VK_END: *result = KEY_END; return 1;
                case VK_DELETE: *result = KEY_DC; return 1;
                case VK_BACK: *result = KEY_BACKSPACE; return 1;
                case VK_RETURN: *result = '\n'; return 1;
                case VK_ESCAPE: *result = 27; return 1;
            }
        }
        /* Some ConPTY input methods place a surrogate on key release.
         * Prefer key down and ignore matching key-up duplicates. */
        if (cp >= 0xd800 && cp <= 0xdbff)
        {
            if (keydown)
                down_high = (WCHAR)cp;
            else if (down_high == (WCHAR)cp)
            {
                down_high = 0;
                continue;
            }
            high = cp;
            continue;
        }
        if (cp >= 0xdc00 && cp <= 0xdfff)
        {
            if (keydown)
                down_low = (WCHAR)cp;
            else if (down_low == (WCHAR)cp)
            {
                down_low = 0;
                continue;
            }
            if (!high)
                continue;
            *result = (int)(0x10000U + ((high - 0xd800U) << 10) +
                            (cp - 0xdc00U));
            return 1;
        }
        if (!keydown || cp == 0)
            continue;
        high = 0;
        *result = (cp == '\r') ? '\n' : (int)cp;
        return 1;
    }
}

/* Reuse the tested 5.4 approach: supplementary-plane UTF-8 characters
 * must be passed to PDCursesMod as complete Unicode scalar values. */
int
jrogue_windows_waddstr_utf8(WINDOW *win, const char *s)
{
    size_t total, used = 0;
    const unsigned char *q;
    char *segment;
    int result = OK;

    if (!win || !s)
        return ERR;
    total = strlen(s);
    segment = (char *)malloc(total + 1);
    if (!segment)
        return waddstr(win, s);
    q = (const unsigned char *)s;
    while (*q)
    {
        int offset = (int)(q - (const unsigned char *)s);
        int next = offset;
        unsigned long cp = jrogue_utf8_codepoint_at(s, (int)total, &next);

        /* Non-BMP scalars occupy a single PDCurses wide cell.  Other
         * sequences stay in a UTF-8 text segment as before. */
        if (next - offset == 4 && cp >= 0x10000UL && cp <= 0x10ffffUL)
        {
            if (used)
            {
                segment[used] = '\0';
                if (waddstr(win, segment) == ERR)
                    result = ERR;
                used = 0;
            }
            if (waddch(win, (chtype)cp) == ERR)
                result = ERR;
            q += 4;
        }
        else
            segment[used++] = (char)*q++;
    }
    if (used)
    {
        segment[used] = '\0';
        if (waddstr(win, segment) == ERR)
            result = ERR;
    }
    free(segment);
    return result;
}

/* PDCursesMod's VT output can narrow a non-BMP cell to 16 bits. Repaint
 * only those cells with full UTF-8 after PDCurses has refreshed its window,
 * restoring the cursor's original position afterwards. */
void
jrogue_windows_option_vt_restore_nonbmp(WINDOW *win)
{
    HANDLE out;
    DWORD written, mode;
    int rows, cols, beg_y, beg_x, cur_y, cur_x, y, x;
    int repainted = 0;
    char output[80];

    if (win != hw)
        return;
    out = GetStdHandle(STD_ERROR_HANDLE);
    if (out == NULL || out == INVALID_HANDLE_VALUE ||
        !GetConsoleMode(out, &mode))
        return;
    getmaxyx(win, rows, cols);
    getbegyx(win, beg_y, beg_x);
    getyx(win, cur_y, cur_x);
    for (y = 0; y < rows; ++y)
        for (x = 0; x < cols; ++x)
        {
            unsigned int cp = (unsigned int)(mvwinch(win, y, x) & A_CHARTEXT);
            char utf8[4];
            int n;
            if (cp < 0x10000U || cp > 0x10ffffU || beg_x + x >= COLS - 1)
                continue;
            utf8[0] = (char)(0xf0 | (cp >> 18));
            utf8[1] = (char)(0x80 | ((cp >> 12) & 0x3f));
            utf8[2] = (char)(0x80 | ((cp >> 6) & 0x3f));
            utf8[3] = (char)(0x80 | (cp & 0x3f));
            n = snprintf(output, sizeof output, "\033[%d;%dH%c%c%c%c",
                         beg_y + y + 1, beg_x + x + 1,
                         utf8[0], utf8[1], utf8[2], utf8[3]);
            if (n > 0 && (size_t)n < sizeof output &&
                WriteFile(out, output, (DWORD)n, &written, NULL))
                repainted = 1;
        }
    /* PDCurses already positioned the cursor.  Do not emit redundant VT
     * cursor movement when the option screen contains no emoji. */
    (void)wmove(win, cur_y, cur_x);
    if (!repainted)
        return;
    {
        int n = snprintf(output, sizeof output, "\033[%d;%dH",
                         beg_y + cur_y + 1, beg_x + cur_x + 1);
        if (n > 0 && (size_t)n < sizeof output)
            (void)WriteFile(out, output, (DWORD)n, &written, NULL);
    }
}
