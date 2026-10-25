/* Internal player configuration and CLI contract; validation never starts a window or VM. */
#ifndef FVM_DESKTOP_OPTIONS_H
#define FVM_DESKTOP_OPTIONS_H
#include "export.h"
#include "flashvm/execution_mode.h"
#include "flashvm/stage.h"

#define FVM_DESKTOP_DECIMAL_MAX_TEXT                                                                                   \
    128U /* Bound CLI numeric scanning independently of locale.                                    \
          */
#define FVM_DESKTOP_DECIMAL_MAX_EXP 400            /* Larger explicit powers cannot reach the accepted range. */
#define FVM_DESKTOP_RATE_NONE 0                    /* No explicit timing option was selected. */
#define FVM_DESKTOP_RATE_FPS 1                     /* Absolute frame frequency option. */
#define FVM_DESKTOP_RATE_PERCENT 2                 /* Authored frequency multiplier option. */
#define FVM_DESKTOP_MODE_AUTO FVM_EXECUTION_MANUAL /* Frontend sentinel; resolves before VM execution. */
#define FVM_DESKTOP_PARSE_ERROR 0                  /* Malformed option or missing movie. */
#define FVM_DESKTOP_PARSE_OK 1                     /* Ready to load the requested playlist. */
#define FVM_DESKTOP_PARSE_VERSION 3                /* Print build metadata without movie/device startup. */
#define FVM_DESKTOP_PARSE_HELP 2                   /* Print help and exit successfully. */
#define FVM_DESKTOP_MAX_FPS 1000.0                 /* Bound scheduler work and timer resolution demands. */
#define FVM_DESKTOP_MIN_RATE 0.1                   /* Smallest positive FPS/percentage argument. */
#define FVM_DESKTOP_MAX_PERCENT 10000.0            /* Maximum authored-rate multiplier in percent. */
#define FVM_DESKTOP_PERCENT_UNIT 100.0             /* Percentage denominator, also default speed. */
#define FVM_DESKTOP_FIXED8_UNIT 256.0              /* Authored SWF FPS fractional denominator. */
#define FVM_DESKTOP_DEFAULT_FPS 0.0                /* Zero means use the authored rate, not a paused clock. */
#define FVM_DESKTOP_MAX_LOADS 1024UL               /* Bound sequential movies, including initial playback. */


/* Borrow path strings; caller owns the file-pointer array until playback returns. */
typedef struct FVM_DesktopOptions
{
    FVM_ExecutionMode mode;   /* Explicit interpreter/JIT, or MANUAL for frontend auto-selection. */
    FVM_StageQuality quality; /* Host starting quality, later changeable by SWF actions. */
    double fps;               /* Explicit absolute rate, or zero for authored-rate scaling. */
    double percent;           /* Percentage of the SWF rate; mutually exclusive with explicit fps. */
    unsigned long max_loads;  /* Resource-load budget, including startup; default one. */
    unsigned long frames;     /* Per-file tick limit, or zero for the normal playback policy. */
    int once;                 /* Stop after the authored frame count; multiple files imply this by default. */
    int finite_audio_tails;   /* Opt-in finite event-only and root mixed tails with completion drain. */
    int finite_mixed_tails;   /* Opt-in finite event-only blocks for root stream/event composition. */
    int mute;                 /* Explicitly skip device opening and event sound for headless/audio-free sessions. */
    int stats;                /* Print actual VM counters after each movie. */
    const char *const *files; /* Caller-owned immutable array of borrowed movie paths. */
    size_t file_count;        /* Number of initialized entries in files. */
} FVM_DesktopOptions;
#ifdef __cplusplus
extern "C"
{
#endif


/* Initialize defaults without allocation; null output is rejected. No files are selected. */
FVM_DESKTOP_API int fvm_desktop_options_init(FVM_DesktopOptions *out);


/* Validate all fields and every borrowed playlist path before any file/device startup.
     * Optional error receives a static message on failure, null on success. No fields change. */
FVM_DESKTOP_API int fvm_desktop_options_validate(const FVM_DesktopOptions *options, const char **error);


/* Check argc and every pointer in the caller-owned argc-element argv array.
     * A trailing argv[argc] sentinel is not read; strings remain borrowed. */
FVM_DESKTOP_API int fvm_desktop_arguments_valid(int argc, char **argv);
/* Parse atomically into out; files is scratch storage with at least argc elements.
     * Return PARSE_HELP/PARSE_VERSION without opening files or touching graphics. */
FVM_DESKTOP_API int
fvm_desktop_parse(int argc, char **argv, const char **files, FVM_DesktopOptions *out, const char **error);


/* Validate authored/effective rate; output is unchanged on failure. */
FVM_DESKTOP_API int fvm_desktop_rate(const FVM_DesktopOptions *, double authored_fps, double *out);


/* Print complete user-facing command-line help to stdout. */
FVM_DESKTOP_API void fvm_desktop_help(const char *program);
#ifdef __cplusplus
}
#endif
#endif
