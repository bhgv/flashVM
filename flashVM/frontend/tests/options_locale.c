/* Poison locale-sensitive conversion: ASCII parser must never call it or change locale. */
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>

static unsigned conversions, locale_changes;


double poisoned_conversion(const char *text, char **end)
{
    ++conversions;
    *end = (char *)text;
    return 0;
}


char *poisoned_locale(int category, const char *name)
{
    (void)category;
    (void)name;
    ++locale_changes;
    return NULL;
}

#define strtod poisoned_conversion
#define setlocale poisoned_locale
#include "../common/options.c"

#undef strtod
#undef setlocale


int main(void)
{
    const char *files[8], *error;
    FVM_DesktopOptions options;
    char *args[] = {"player", "--fps", "12.5", "movie.swf"};
    const char *good[] = {"12.5", "125e-1", ".125e2", "1.25E+1", "0012.500"};
    const char *bad[] = {"12,5", "1.2.3", ".e1", "1e", "1e+", "1e++2", "1e401", "1e-401", "+12.5", "12.5%", ".0"};
    unsigned i;
    char long_value[FVM_DESKTOP_DECIMAL_MAX_TEXT + 2];
    for (i = 0; i < sizeof(good) / sizeof(good[0]); ++i)
    {
        args[2] = (char *)good[i];
        if (!(fvm_desktop_parse(4, args, files, &options, &error) == FVM_DESKTOP_PARSE_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(options.fps > 12.499999 && options.fps < 12.500001))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i)
    {
        args[2] = (char *)bad[i];
        if (!(fvm_desktop_parse(4, args, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    memset(long_value, '0', sizeof(long_value));
    long_value[sizeof(long_value) - 1] = 0;
    args[2] = long_value;
    if (!(fvm_desktop_parse(4, args, files, &options, &error) == FVM_DESKTOP_PARSE_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    args[1] = "--speed";
    args[2] = "125e-1%";
    if (!(fvm_desktop_parse(4, args, files, &options, &error) == FVM_DESKTOP_PARSE_OK && options.percent == 12.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!conversions && !locale_changes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
