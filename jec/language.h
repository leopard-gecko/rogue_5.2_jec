#ifndef JROGUE_LANGUAGE_H
#define JROGUE_LANGUAGE_H

/*
 * Initial language precedence (same as Rogue 5.4.5 Japanese/English):
 *   1. ROGUEOPTS=language=ja|en|auto (parsed in main.c)
 *   2. DEFAULT_LANGUAGE: 0=auto, 1=Japanese, 2=English
 *   3. When auto is selected, OS locale (LC_ALL, LC_MESSAGES, LANG on
 *      macOS/Linux; Windows UI language on Windows).
 *
 * "auto" is an initial-selection mode only. The running game has only
 * Japanese and English, selectable by J/E from the option screen.
 */
#ifndef DEFAULT_LANGUAGE
#define DEFAULT_LANGUAGE 0
#endif

/*
 * DEFAULT_JAPANESE is derived internally from DEFAULT_LANGUAGE.
 * External build rules should define DEFAULT_LANGUAGE only; deriving this
 * helper here avoids inconsistent language defaults in static initializers.
 */
#ifdef DEFAULT_JAPANESE
#undef DEFAULT_JAPANESE
#endif
#if DEFAULT_LANGUAGE == 1
#define DEFAULT_JAPANESE 1
#else
#define DEFAULT_JAPANESE 0
#endif

extern int jrogue_lang_is_japanese;

void jrogue_detect_language(void);
void jrogue_apply_language(void);
void jrogue_apply_options_language(void);
void jrogue_apply_rip_language(void);
const char *jrogue_str(const char *ja, const char *en);
int jrogue_chr(int ja, int en);
const char *jrogue_language_code(void);
int jrogue_set_language(const char *lang);

/* Use these in normal code. */
#define JSTR(ja, en) ((char *) jrogue_str((ja), (en)))
#define JCHR(ja, en) jrogue_chr((ja), (en))

/*
 * Use these only in static initializers, where C requires compile-time
 * constant expressions.
 */
#if DEFAULT_JAPANESE
# define JSTR_STATIC(ja, en) ja
# define JCHR_STATIC(ja, en) ja
#else
# define JSTR_STATIC(ja, en) en
# define JCHR_STATIC(ja, en) en
#endif

#endif /* JROGUE_LANGUAGE_H */
