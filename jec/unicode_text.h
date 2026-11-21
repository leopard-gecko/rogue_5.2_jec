/*
 * UTF-8 byte boundaries and terminal-cell widths shared by Rogue 5.2.
 * This module is independent of curses and operating-system APIs.
 * See LICENSE.TXT for copyright and licensing information.
 * The width table intentionally preserves the established JEC behavior;
 * more extensive grapheme/emoji handling is a separate future change.
 */
#ifndef ROGUE52_UNICODE_TEXT_H
#define ROGUE52_UNICODE_TEXT_H

/* Returns 0 for an invalid UTF-8 leading byte. */
int jrogue_utf8_expected_len(unsigned char c);
int jrogue_utf8_continuation(unsigned char c);
int jrogue_utf8_prev_index(const char *s, int pos);
int jrogue_utf8_next_index(const char *s, int len, int pos);
unsigned long jrogue_utf8_codepoint_at(const char *s, int len, int *posp);
int jrogue_unicode_display_width(unsigned long cp);
int jrogue_utf8_display_width_n(const char *s, int nbytes);
int jrogue_utf8_copyable_byte(unsigned char c);

/* Convert a Unicode scalar to a NUL-terminated UTF-8 sequence.
 * The caller provides at least five bytes. Returns 0 for invalid scalars.
 * Shared by the Windows console input and any future platform reader. */
int jrogue_utf8_encode(unsigned long cp, char out[5]);

/* Existing Rogue-facing entry points, kept for source compatibility. */
void utf8_trim_partial(char *s);
void utf8_display_truncate(char *dst, const char *src, int maxwidth, int dstsize);
int rogue_display_width(const char *s);
void utf8_strlcpy(char *dst, const char *src, int dstsize);

#endif /* ROGUE52_UNICODE_TEXT_H */
