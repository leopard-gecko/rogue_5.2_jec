#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "language.h"

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* The runtime state is always Japanese or English, never "auto". */
int jrogue_lang_is_japanese = DEFAULT_JAPANESE;

/* ROGUEOPTS uses the explicit, unambiguous ja/en/auto values. */
static int
lang_equals(const char *value, const char *expected)
{
    unsigned char a, b;

    if (value == NULL || expected == NULL)
        return FALSE;
    for (;;)
    {
        a = (unsigned char)*value++;
        b = (unsigned char)*expected++;
        if (tolower(a) != tolower(b))
            return FALSE;
        if (a == '\0')
            return TRUE;
    }
}

/* Only "auto" consults the operating-system language. */
static int
os_language_is_japanese(void)
{
#ifdef _WIN32
    return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_JAPANESE;
#else
    const char *locale;

    locale = getenv("LC_ALL");
    if (locale == NULL || *locale == '\0')
        locale = getenv("LC_MESSAGES");
    if (locale == NULL || *locale == '\0')
        locale = getenv("LANG");

    /* ja, ja_JP.UTF-8, ja-JP etc. Other locales default to English. */
    return locale != NULL
        && (locale[0] == 'j' || locale[0] == 'J')
        && (locale[1] == 'a' || locale[1] == 'A')
        && (locale[2] == '\0' || locale[2] == '_' ||
            locale[2] == '-' || locale[2] == '.');
#endif
}

void
jrogue_detect_language(void)
{
    /* ROGUEOPTS is handled after this in main.c / parse_opts(). */
#if DEFAULT_LANGUAGE == 1
    jrogue_lang_is_japanese = TRUE;
#elif DEFAULT_LANGUAGE == 2
    jrogue_lang_is_japanese = FALSE;
#else
    jrogue_lang_is_japanese = os_language_is_japanese();
#endif
}

const char *
jrogue_str(const char *ja, const char *en)
{
    return jrogue_lang_is_japanese ? ja : en;
}

int
jrogue_chr(int ja, int en)
{
    return jrogue_lang_is_japanese ? ja : en;
}

const char *
jrogue_language_code(void)
{
    return jrogue_lang_is_japanese ? "ja" : "en";
}

int
jrogue_set_language(const char *lang)
{
    if (lang_equals(lang, "ja"))
        jrogue_lang_is_japanese = TRUE;
    else if (lang_equals(lang, "en"))
        jrogue_lang_is_japanese = FALSE;
    else if (lang_equals(lang, "auto"))
        jrogue_lang_is_japanese = os_language_is_japanese();
    else
        return FALSE;
    return TRUE;
}
