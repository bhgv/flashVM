/* C89 CLI parsing independent of SDL and VM lifetime. Numeric syntax uses the C locale. */
#include "options.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Accept a finite, complete decimal argument in the documented inclusive range. */
static int decimal(const char *text, double maximum, int percent, double *out)
{
    char *end;
    const char *scan;
    double value;
    if (!text || !*text || *text == '-' || *text == '+' || *text == ' ' || *text == '\t')
        return 0;
    for (scan = text; *scan; ++scan)
        if (!((*scan >= '0' && *scan <= '9') || *scan == '.' || *scan == 'e' || *scan == 'E' ||
              *scan == '+' || *scan == '-' || (percent && *scan == '%' && !scan[1])))
            return 0;
    errno = 0;
    value = strtod(text, &end);
    if (errno || (*end && !(percent && *end == '%' && !end[1])) || value != value ||
        value < FVM_DESKTOP_MIN_RATE || value > maximum)
        return 0;
    *out = value;
    return 1;
}

/* Parse a positive frame limit; reject signs, fractions, suffixes and overflow. */
static int frame_limit(const char *text, unsigned long *out)
{
    const char *p;
    char *end;
    unsigned long value;
    if (!text || !*text)
        return 0;
    for (p = text; *p; ++p)
        if (*p < '0' || *p > '9')
            return 0;
    errno = 0;
    value = strtoul(text, &end, 10);
    if (errno || *end || !value)
        return 0;
    *out = value;
    return 1;
}

/* Keep every failure message stable and usable before any SDL initialization. */
int fvm_desktop_parse(int argc, char **argv, const char **files, FVM_DesktopOptions *out,
                      const char **error)
{
    FVM_DesktopOptions value;
    int i, paths_only = 0, rate_kind = FVM_DESKTOP_RATE_NONE, explicit_mode = 0;
    const char *message = "invalid command line";
    if (!argv || !files || !out || !error || argc < 1)
        return FVM_DESKTOP_PARSE_ERROR;
    memset(&value, 0, sizeof(value));
    value.mode = FVM_DESKTOP_MODE_AUTO;
    value.quality = FVM_STAGE_QUALITY_HIGH;
    value.percent = FVM_DESKTOP_PERCENT_UNIT;
    value.files = files;
    *error = 0;
    for (i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (!paths_only && (!strcmp(arg, "-h") || !strcmp(arg, "--help")))
            return FVM_DESKTOP_PARSE_HELP;
        if (!paths_only && !strcmp(arg, "--"))
        {
            paths_only = 1;
            continue;
        }
        if (!paths_only &&
            (!strcmp(arg, "--jit") || !strcmp(arg, "--no-jit") || !strcmp(arg, "--interpreter")))
        {
            FVM_ExecutionMode mode =
                !strcmp(arg, "--jit") ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
            if (explicit_mode && mode != value.mode)
            {
                message = "--jit conflicts with --no-jit/--interpreter";
                goto failed;
            }
            value.mode = mode;
            explicit_mode = 1;
        }
        else if (!paths_only && (!strcmp(arg, "--fps") || !strcmp(arg, "--speed")))
        {
            int kind = !strcmp(arg, "--fps") ? FVM_DESKTOP_RATE_FPS : FVM_DESKTOP_RATE_PERCENT;
            double number;
            if (i + 1 == argc)
            {
                message = "--fps/--speed requires a number";
                goto failed;
            }
            if (rate_kind && rate_kind != kind)
            {
                message = "--fps and --speed are mutually exclusive";
                goto failed;
            }
            if (!decimal(argv[++i],
                         kind == FVM_DESKTOP_RATE_FPS ? FVM_DESKTOP_MAX_FPS
                                                      : FVM_DESKTOP_MAX_PERCENT,
                         kind == FVM_DESKTOP_RATE_PERCENT, &number))
            {
                message = "invalid --fps/--speed value";
                goto failed;
            }
            rate_kind = kind;
            if (kind == FVM_DESKTOP_RATE_FPS)
                value.fps = number;
            else
                value.percent = number;
        }
        else if (!paths_only && !strcmp(arg, "--quality"))
        {
            const char *quality;
            if (++i == argc)
            {
                message = "--quality requires low, medium, high or best";
                goto failed;
            }
            quality = argv[i];
            if (!strcmp(quality, "low"))
                value.quality = FVM_STAGE_QUALITY_LOW;
            else if (!strcmp(quality, "medium"))
                value.quality = FVM_STAGE_QUALITY_MEDIUM;
            else if (!strcmp(quality, "high"))
                value.quality = FVM_STAGE_QUALITY_HIGH;
            else if (!strcmp(quality, "best"))
                value.quality = FVM_STAGE_QUALITY_BEST;
            else
            {
                message = "unknown graphics quality";
                goto failed;
            }
        }
        else if (!paths_only && !strcmp(arg, "--frames"))
        {
            if (++i == argc || !frame_limit(argv[i], &value.frames))
            {
                message = "--frames requires a positive integer";
                goto failed;
            }
        }
        else if (!paths_only && !strcmp(arg, "--once"))
            value.once = 1;
        else if (!paths_only && !strcmp(arg, "--mute"))
            value.mute = 1;
        else if (!paths_only && !strcmp(arg, "--stats"))
            value.stats = 1;
        else if (!paths_only && *arg == '-')
        {
            message = "unknown option (use -- before a dash-prefixed filename)";
            goto failed;
        }
        else if (!*arg)
        {
            message = "empty SWF filename";
            goto failed;
        }
        else
            files[value.file_count++] = arg;
    }
    if (!value.file_count)
    {
        message = "at least one SWF file is required";
        goto failed;
    }
    if (value.frames && value.once)
    {
        message = "--frames conflicts with --once";
        goto failed;
    }
    *out = value;
    return FVM_DESKTOP_PARSE_OK;
failed:
    *error = message;
    return FVM_DESKTOP_PARSE_ERROR;
}

/* Timing policy: absolute FPS overrides the file rate only when explicitly requested. */
int fvm_desktop_rate(const FVM_DesktopOptions *options, double authored, double *out)
{
    double rate;
    if (!options || !out)
        return 0;
    rate = options->fps ? options->fps : authored * options->percent / FVM_DESKTOP_PERCENT_UNIT;
    if (rate != rate || rate <= 0 || rate > FVM_DESKTOP_MAX_FPS)
        return 0;
    *out = rate;
    return 1;
}

/* Keep help exhaustive, independent of the display server and readable in a terminal. */
void fvm_desktop_help(const char *program)
{
    printf("Usage: %s [options] FILE.swf [FILE.swf ...]\n", program);
    puts("  -h, --help              Show this help; no files/display are opened");
    puts("  --jit                   Automatic JIT with per-body interpreter fallback");
    puts("  --no-jit, --interpreter  Force interpreter execution");
    puts("  --quality LEVEL         low | medium | high | best (default: high)");
    puts("  --fps N                 Absolute movie ticks/second, 0.1..1000");
    puts("  --speed N[%]            Percent of SWF FPS, 0.1..10000 (default: 100)");
    puts("                          --fps and --speed cannot be combined");
    puts("  --once                  Run one authored frame-count worth of ticks");
    puts("  --frames N              Limit ticks per file; cannot combine with --once");
    puts("  --mute                  Disable event-sound device opening/playback");
    puts("  --stats                 Print interpreter/JIT and presentation counters after playback");
    puts("  --                      Treat remaining arguments as filenames");
    puts("Single-file playback loops until window close/Escape; playlists run each file once.");
    puts("HostInput exposes logical pointer/buttons, arrows/space, key transitions and counters.");
    puts("Controls: Escape/window close exits the entire playlist. Resize preserves aspect.");
    puts("Touch: the first contact drives the pointer; additional contacts are ignored.");
    puts("This frontend plays the currently supported SWF subset; root PCM/ADPCM StartSound uses "
         "optional SDL audio.");
}
