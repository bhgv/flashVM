/* C89 CLI parsing independent of SDL/VM lifetime and process numeric locale. */
#include "audio_memory.h"
#include "options.h"
#include <errno.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Parse ASCII decimal syntax without strtod/setlocale or process-global state.
 * Rates are presentation doubles, not an exact decimal arithmetic/wire format. */
static int decimal(const char *text, double maximum, int percent, double *out)
{
    const char *scan;
    size_t length = 0;
    double value = 0;
    int digits = 0, fractional = 0, point = 0, exponent = 0, negative = 0;
    if (!text || !*text)
        return 0;
    for (scan = text; *scan; ++scan)
        if (++length > FVM_DESKTOP_DECIMAL_MAX_TEXT)
            return 0;
    scan = text;
    while ((*scan >= '0' && *scan <= '9') || *scan == '.')
    {
        if (*scan == '.')
        {
            if (point)
                return 0;
            point = 1;
        }
        else
        {
            unsigned int digit = (unsigned int)(*scan - '0');
            if (value > (DBL_MAX - digit) / 10)
                return 0;
            value = value * 10 + digit;
            ++digits;
            if (point)
                ++fractional;
        }
        ++scan;
    }
    if (!digits)
        return 0;
    if (*scan == 'e' || *scan == 'E')
    {
        ++scan;
        if (*scan == '+' || *scan == '-')
        {
            negative = *scan == '-';
            ++scan;
        }
        if (*scan < '0' || *scan > '9')
            return 0;
        while (*scan >= '0' && *scan <= '9')
        {
            exponent = exponent * 10 + (*scan++ - '0');
            if (exponent > FVM_DESKTOP_DECIMAL_MAX_EXP)
                return 0;
        }
    }
    if (*scan == '%' && percent)
        ++scan;
    if (*scan)
        return 0;
    exponent = (negative ? -exponent : exponent) - fractional;
    while (exponent > 0)
    {
        if (value > maximum / 10)
            return 0;
        value *= 10;
        --exponent;
    }
    while (exponent < 0)
    {
        value /= 10;
        if (value < FVM_DESKTOP_MIN_RATE)
            return 0;
        ++exponent;
    }
    if (value < FVM_DESKTOP_MIN_RATE || value > maximum)
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

/* Validate pointers before parsing, help printing or argv-storage allocation.
 * The caller must still supply an accessible array and terminated argument strings. */
int fvm_desktop_arguments_valid(int argc, char **argv)
{
    int i;
    if (argc < 1 || !argv ||
        (unsigned long)argc > (unsigned long)((size_t)-1 / sizeof(const char *)))
        return 0;
    for (i = 0; i < argc; ++i)
        if (!argv[i])
            return 0;
    return 1;
}

/* Default state shared by CLI parsing and direct C/C++ embedding. */
int fvm_desktop_options_init(FVM_DesktopOptions *out)
{
    if (!out)
        return 0;
    memset(out, 0, sizeof(*out));
    out->mode = FVM_DESKTOP_MODE_AUTO;
    out->quality = FVM_STAGE_QUALITY_HIGH;
    out->percent = FVM_DESKTOP_PERCENT_UNIT;
    out->max_loads = 1;
    return 1;
}

/* Validate the entire configuration before even the first playlist file is opened. */
int fvm_desktop_options_validate(const FVM_DesktopOptions *options, const char **error)
{
    const char *message = 0;
    size_t i;
    if (!options)
        message = "null player options";
    else if (options->mode != FVM_DESKTOP_MODE_AUTO && options->mode != FVM_EXECUTION_INTERPRETER &&
             options->mode != FVM_EXECUTION_JIT)
        message = "invalid execution mode";
    else if (options->quality < FVM_STAGE_QUALITY_LOW || options->quality > FVM_STAGE_QUALITY_BEST)
        message = "invalid stage quality";
    else if (!(options->fps == FVM_DESKTOP_DEFAULT_FPS ||
               (options->fps >= FVM_DESKTOP_MIN_RATE && options->fps <= FVM_DESKTOP_MAX_FPS)))
        message = "invalid absolute frame rate";
    else if (!(options->percent >= FVM_DESKTOP_MIN_RATE &&
               options->percent <= FVM_DESKTOP_MAX_PERCENT))
        message = "invalid frame rate percentage";
    else if (options->fps != FVM_DESKTOP_DEFAULT_FPS &&
             options->percent != FVM_DESKTOP_PERCENT_UNIT)
        message = "absolute frame rate conflicts with percentage";
    else if ((options->once != 0 && options->once != 1) ||
             (options->mute != 0 && options->mute != 1) ||
             (options->stats != 0 && options->stats != 1))
        message = "player flags must be zero or one";
    else if (options->once && options->frames)
        message = "once conflicts with explicit frame limit";
    else if (!options->max_loads || options->max_loads > FVM_DESKTOP_MAX_LOADS)
        message = "invalid root movie load limit";
    else if (!options->files || !options->file_count ||
             options->file_count > (size_t)-1 / sizeof(*options->files))
        message = "invalid playlist storage or count";
    else
        for (i = 0; i < options->file_count; ++i)
            if (!options->files[i] || !options->files[i][0])
            {
                message = "null or empty movie path";
                break;
            }
    if (error)
        *error = message;
    return message == 0;
}

/* Keep every failure message stable and usable before any SDL initialization. */
int fvm_desktop_parse(int argc, char **argv, const char **files, FVM_DesktopOptions *out,
                      const char **error)
{
    FVM_DesktopOptions value;
    int i, paths_only = 0, rate_kind = FVM_DESKTOP_RATE_NONE, explicit_mode = 0;
    const char *message = "invalid command line";
    if (error)
        *error = "invalid argument vector or parser storage";
    if (!files || !out || !error || !fvm_desktop_arguments_valid(argc, argv))
        return FVM_DESKTOP_PARSE_ERROR;
    fvm_desktop_options_init(&value);
    value.files = files;
    *error = 0;
    for (i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (!paths_only && (!strcmp(arg, "-h") || !strcmp(arg, "--help")))
            return FVM_DESKTOP_PARSE_HELP;
        if (!paths_only && !strcmp(arg, "--version"))
            return FVM_DESKTOP_PARSE_VERSION;
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
        else if (!paths_only && !strcmp(arg, "--loads"))
        {
            if (++i == argc || !frame_limit(argv[i], &value.max_loads) ||
                value.max_loads > FVM_DESKTOP_MAX_LOADS)
            {
                message = "--loads requires 1..1024 total resource loads";
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
    puts("  --version               Show build metadata without opening files/devices");
    puts("  --jit                   Automatic JIT with per-body interpreter fallback");
    puts("  --no-jit, --interpreter  Force interpreter execution");
    puts("  --quality LEVEL         low | medium | high | best (default: high)");
    puts("  --fps N                 Absolute movie ticks/second, 0.1..1000");
    puts("  --speed N[%]            Percent of SWF FPS, 0.1..10000 (default: 100)");
    puts("                          --fps and --speed cannot be combined");
    puts(
        "  --async-loads           Keep timeline/timers running during network child/form fetches");
    puts("  --background-loads      Fetch network bytes on joined workers; pump the active window");
    puts("  --http-ca-file PATH     Explicit PEM trust for every TLS hop; requires a worker load mode");
    puts("  --redirects N           Explicit HTTP GET redirect profile, 0..8 transitions per load");
    puts("                          Requires --async-loads or --background-loads; default disabled");
    puts("  --load-timeout MS       Total cooperative job budget, 0..2147483647 (0 disables)");
    puts("  --load-progress         Log owner-thread network request phases and body bytes");
    puts("                          Both require --async-loads or --background-loads");
    puts("  --loads N               Allow 1..1024 resource loads, including startup (default 1)");
    puts("  --once                  Run one authored frame-count worth of ticks");
    puts("  --frames N              Limit ticks per file; cannot combine with --once");
    puts("  --mute                  Mute event sound and validated root streams; no device");
    puts(
        "  --stats                 Print interpreter/JIT and presentation counters after playback");
    puts("  --                      Treat remaining arguments as filenames");
    puts("Network input requires an optional CURL or SOCKET_HTTP provider; redirects require explicit policy; no POST.");
    puts("Single-file playback loops until window close/Escape; playlists run each file once.");
    puts("HostInput exposes logical pointer/buttons, arrows/space, key transitions and counters.");
    puts("Controls: Escape/window close exits the entire playlist. Resize preserves aspect.");
    puts("Touch: the first contact drives the pointer; additional contacts are ignored.");
    puts("This frontend plays the currently supported SWF subset; root PCM/ADPCM/MP3 StartSound uses "
         "optional SDL audio.");
    puts("Finite stream playback finishes authorized PCM and drains the queue, with a 2000 ms extra deadline.");
    printf("Prepared streaming sources: %lu-byte aggregate snapshot/index/table and expansion budget.\n",
           (unsigned long)FVM_DESKTOP_AUDIO_PREPARED_BYTES);
    puts("Root streaming audio: up to 64 PCM/whole-packet ADPCM/supported MP3 segments without StartSound; other codecs/mixing unsupported.");
}
