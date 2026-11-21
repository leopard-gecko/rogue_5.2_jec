/*
 * Shared UTF-8 handling for Rogue 5.2 names, messages and option editing.
 * No curses or operating-system dependencies: usable on all target OSes.
 * See LICENSE.TXT for copyright and licensing information.
 * Keep the 003 codepoint width table and valid-UTF-8 editing behavior.
 */
#include <string.h>
#include "unicode_text.h"

int
jrogue_utf8_expected_len(unsigned char c)
{
    if (c < 0x80) return 1;
    if (c >= 0xc2 && c <= 0xdf) return 2;
    if (c >= 0xe0 && c <= 0xef) return 3;
    if (c >= 0xf0 && c <= 0xf4) return 4;
    return 0;
}

int
jrogue_utf8_continuation(unsigned char c)
{
    return ((c & 0xc0) == 0x80);
}

/* A malformed leading byte or broken sequence is treated as one byte,
 * as in the former io.c implementation.  In well-formed UTF-8 the
 * result is identical to the old io.c and options.c decoders. */
unsigned long
jrogue_utf8_codepoint_at(const char *s, int len, int *posp)
{
    unsigned char c;
    unsigned long cp;
    int pos, need, i;

    pos = *posp;
    if (pos >= len) return 0;
    c = (unsigned char)s[pos];
    need = jrogue_utf8_expected_len(c);
    if (need <= 1 || pos + need > len)
    {
        *posp = pos + 1;
        return c;
    }
    for (i = 1; i < need; i++)
        if (!jrogue_utf8_continuation((unsigned char)s[pos + i]))
        {
            *posp = pos + 1;
            return c;
        }

    if (need == 2)
        cp = ((unsigned long)(c & 0x1f) << 6)
           | ((unsigned long)(s[pos + 1] & 0x3f));
    else if (need == 3)
        cp = ((unsigned long)(c & 0x0f) << 12)
           | ((unsigned long)(s[pos + 1] & 0x3f) << 6)
           | ((unsigned long)(s[pos + 2] & 0x3f));
    else
        cp = ((unsigned long)(c & 0x07) << 18)
           | ((unsigned long)(s[pos + 1] & 0x3f) << 12)
           | ((unsigned long)(s[pos + 2] & 0x3f) << 6)
           | ((unsigned long)(s[pos + 3] & 0x3f));

    *posp = pos + need;
    return cp;
}

int
jrogue_unicode_display_width(unsigned long cp)
{
    if (cp == 0) return 0;
    if (cp < 0x20 || (cp >= 0x7f && cp < 0xa0)) return 0;
    if ((cp >= 0x1100 && cp <= 0x115f) ||
        (cp >= 0x2329 && cp <= 0x232a) ||
        (cp >= 0x2e80 && cp <= 0xa4cf) ||
        (cp >= 0xac00 && cp <= 0xd7a3) ||
        (cp >= 0xf900 && cp <= 0xfaff) ||
        (cp >= 0xfe10 && cp <= 0xfe19) ||
        (cp >= 0xfe30 && cp <= 0xfe6f) ||
        (cp >= 0xff00 && cp <= 0xff60) ||
        (cp >= 0xffe0 && cp <= 0xffe6) ||
        (cp >= 0x1f300 && cp <= 0x1faff) ||
        (cp >= 0x20000 && cp <= 0x3fffd))
        return 2;
    return 1;
}

int
jrogue_utf8_prev_index(const char *s, int pos)
{
    int p;
    if (pos <= 0) return 0;
    p = pos - 1;
    while (p > 0 && jrogue_utf8_continuation((unsigned char)s[p])) p--;
    return p;
}

int
jrogue_utf8_next_index(const char *s, int len, int pos)
{
    int need;
    if (pos >= len) return len;
    need = jrogue_utf8_expected_len((unsigned char)s[pos]);
    if (need <= 0) need = 1;
    if (pos + need > len) return len;
    return pos + need;
}

int
jrogue_utf8_display_width_n(const char *s, int nbytes)
{
    int pos, width;
    if (s == NULL || nbytes <= 0) return 0;
    pos = width = 0;
    while (pos < nbytes && s[pos] != '\0')
        width += jrogue_unicode_display_width(
            jrogue_utf8_codepoint_at(s, nbytes, &pos));
    return width;
}

int
rogue_display_width(const char *s)
{
    if (s == NULL) return 0;
    return jrogue_utf8_display_width_n(s, (int)strlen(s));
}

void
utf8_trim_partial(char *s)
{
    int len, start, need;
    if (s == NULL) return;
    len = strlen(s);
    if (len <= 0) return;
    start = len - 1;
    while (start > 0 && jrogue_utf8_continuation((unsigned char)s[start]))
        start--;
    need = jrogue_utf8_expected_len((unsigned char)s[start]);
    if (need <= 0) need = 1;
    if (start + need > len) s[start] = '\0';
}

void
utf8_display_truncate(char *dst, const char *src, int maxwidth, int dstsize)
{
    int len, pos, out, next, width, w;
    if (dstsize <= 0) return;
    dst[0] = '\0';
    if (src == NULL || maxwidth <= 0) return;

    len = strlen(src);
    pos = out = width = 0;
    while (pos < len && src[pos] != '\0')
    {
        next = pos;
        w = jrogue_unicode_display_width(
            jrogue_utf8_codepoint_at(src, len, &next));
        if (width + w > maxwidth) break;
        if (out + (next - pos) >= dstsize) break;
        memcpy(dst + out, src + pos, (size_t)(next - pos));
        out += next - pos;
        width += w;
        pos = next;
    }
    dst[out] = '\0';
    utf8_trim_partial(dst);
}

/* Encode one complete Unicode scalar. Keep input/encoding policy in the
 * portable UTF-8 module, not in the platform-specific console reader. */
int
jrogue_utf8_encode(unsigned long cp, char out[5])
{
    if (out == NULL || cp > 0x10ffffUL ||
        (cp >= 0xd800UL && cp <= 0xdfffUL))
        return 0;
    if (cp < 0x80UL)
    {
        out[0] = (char)cp;
        out[1] = '\0';
        return 1;
    }
    if (cp < 0x800UL)
    {
        out[0] = (char)(0xc0UL | (cp >> 6));
        out[1] = (char)(0x80UL | (cp & 0x3f));
        out[2] = '\0';
        return 2;
    }
    if (cp < 0x10000UL)
    {
        out[0] = (char)(0xe0UL | (cp >> 12));
        out[1] = (char)(0x80UL | ((cp >> 6) & 0x3f));
        out[2] = (char)(0x80UL | (cp & 0x3f));
        out[3] = '\0';
        return 3;
    }
    out[0] = (char)(0xf0UL | (cp >> 18));
    out[1] = (char)(0x80UL | ((cp >> 12) & 0x3f));
    out[2] = (char)(0x80UL | ((cp >> 6) & 0x3f));
    out[3] = (char)(0x80UL | (cp & 0x3f));
    out[4] = '\0';
    return 4;
}

int
jrogue_utf8_copyable_byte(unsigned char c)
{
    if (c >= 0x80 || c == ' ' || (c >= 0x21 && c <= 0x7e))
        return 1;
    return 0;
}

void
utf8_strlcpy(char *dst, const char *src, int dstsize)
{
    int copied;
    if (dstsize <= 0) return;
    dst[0] = '\0';
    if (src == NULL) return;
    copied = 0;
    while (*src && copied < dstsize - 1)
    {
        unsigned char c = (unsigned char)*src++;
        if (jrogue_utf8_copyable_byte(c)) dst[copied++] = (char)c;
    }
    dst[copied] = '\0';
    utf8_trim_partial(dst);
}
