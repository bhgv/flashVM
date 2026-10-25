/* C89 playback commands without a console dependency; contracts: README.md / README.ru.md. */
#include "../../../frontend/common/player.h"      /* Public player host ABI, with no SDL types. */
#ifdef FVM_DESKTOP_SOCKET_HTTP                    /* Prefer the explicitly configured socket transport. */
#include "../../../frontend/common/socket_http.h" /* Worker-safe socket provider declaration. */
#elif defined(FVM_DESKTOP_HTTP)                   /* A different build may select libcurl instead. */
#include "../../../frontend/common/http.h"        /* Worker-safe curl provider declaration. */
#endif                                            /* No transport is required for local or embedded movies. */
#include <stdio.h>                                /* Report copied counters and borrowed errors. */
#include <string.h>                               /* Compare this executable's small command-line vocabulary. */
#include "movie.inc"                              /* Immutable SWF bytes whose lifetime covers the complete run. */
#define EXAMPLE_CONTROL_MAX_STEPS 10000UL         /* Bound the tutorial's decimal argument and frame count. */
#define EXAMPLE_CONTROL_PAUSE_MS 150.0            /* Demonstrate owner services during a brief timeline pause. */
#define EXAMPLE_CONTROL_RUN_TICKS 5UL             /* Default sequence ends after five completed host advances. */
#define EXAMPLE_CONTROL_LOADS 8UL                 /* Bound startup plus child/form resource requests. */
#define EXAMPLE_CONTROL_TIMEOUT_MS 5000UL         /* Network jobs must cooperate with this deadline. */

/* Host-owned scalar state; no managed pointer survives the callback that supplies it. */
typedef struct ExampleControl /* An IDE can own the same structure without argv parsing. */
{                             /* Record commands and results independently of the VM's heap. */
    unsigned long steps;      /* Nonzero selects a bounded all-single-step run. */
    unsigned long completed;  /* Copy the last completed tick count for reporting. */
    double held_since;        /* Monotonic pause start copied from the owner's snapshot. */
} ExampleControl;             /* Stack storage remains live until the synchronous runner returns. */


/* Translate one safe owner notification into one command; see PLAYBACK_CONTROL.md. */
static FVM_DesktopPlaybackCommand observe(const FVM_DesktopPlaybackState *state, void *userdata) /* No worker calls. */
{                                                                                                /* The runner pins root; this example only reads scalar snapshot fields. */
    ExampleControl *host = (ExampleControl *)userdata;                                           /* Borrow our caller-owned state. */
    host->completed = state->total_ticks;                                                        /* Copy data rather than retaining state or root. */
    if (state->phase == FVM_DESKTOP_PLAYBACK_READY)                                              /* Each imported root starts before its first tick. */
    {                                                                                            /* Begin conditional branch: Each imported root starts before its first tick. */
        return FVM_DESKTOP_PLAYBACK_STEP;                                                        /* Initial pause grants exactly one host advance. */
    } /* End conditional branch: Each imported root starts before its first tick. */
    if (state->phase == FVM_DESKTOP_PLAYBACK_TICK)                                         /* The current tick and presentation succeeded. */
    {                                                                                      /* Commands affect future advances, never rewind the completed frame. */
        if (state->total_ticks >= (host->steps ? host->steps : EXAMPLE_CONTROL_RUN_TICKS)) /* Bound both modes. */
        {                                                                                  /* Begin conditional branch: Bound both modes. */
            return FVM_DESKTOP_PLAYBACK_STOP;                                              /* Stop the entire run, including later root navigation. */
        } /* End conditional branch: Bound both modes. */
        if (host->steps)                      /* Batch stepping is independent of the authored FPS deadline. */
        {                                     /* Begin conditional branch: Batch stepping is independent of the authored FPS deadline. */
            return FVM_DESKTOP_PLAYBACK_STEP; /* Stay paused while granting one more advance. */
        } /* End conditional branch: Batch stepping is independent of the authored FPS deadline. */
        if (state->total_ticks == 1)              /* Default mode first demonstrates a real paused interval. */
        {                                         /* Use the player's monotonic observation clock, not a blocking sleep. */
            host->held_since = state->elapsed_ms; /* Preserve the pause boundary as a scalar. */
            return FVM_DESKTOP_PLAYBACK_PAUSE;    /* Input/timers/loading/redraw remain serviced. */
        } /* The second completed tick transitions into normal continuous playback. */
        return FVM_DESKTOP_PLAYBACK_RESUME; /* Resume without catching up paused time. */
    } /* TURN notifications can decide when to grant the second step. */
    if (!host->steps && state->paused && state->total_ticks == 1 &&       /* Test the default paused phase. */
        state->elapsed_ms - host->held_since >= EXAMPLE_CONTROL_PAUSE_MS) /* Owner turns keep running. */
    {                                                                     /* Begin scope: Owner turns keep running. */
        return FVM_DESKTOP_PLAYBACK_STEP;                                 /* Advance once; the next TICK requests RESUME. */
    } /* End scope: Owner turns keep running. */
    return FVM_DESKTOP_PLAYBACK_KEEP; /* Preserve current state on ordinary owner turns. */
} /* Snapshot, context/root and source pointers cease to be borrowed here. */


/* Parse a positive bounded decimal without locale, overflow, signs or partial acceptance. */
static int parse_steps(const char *text, unsigned long *out) /* This adapter does not belong to the library. */
{                                                            /* Accumulate only after validating each digit and the remaining bound. */
    unsigned long value = 0;                                 /* Zero is rejected below instead of meaning unlimited. */
    if (!text || !*text)                                     /* Condition for this action: Require at least one decimal digit. */
    {                                                        /* Begin conditional branch: Condition for this action: Require at least one decimal digit. */
        return 0;                                            /* Require at least one decimal digit. */
    } /* End conditional branch: Condition for this action: Require at least one decimal digit. */
    while (*text)                       /* Inspect the complete argument, rejecting trailing text. */
    {                                   /* The maximum bounds multiplication before it can overflow. */
        unsigned long digit;            /* C89 declaration precedes statements in this block. */
        if (*text < '0' || *text > '9') /* Condition for this action: Signs, whitespace and non-ASCII digits fail. */
        {                               /* Begin conditional branch: Condition for this action: Signs, whitespace and non-ASCII digits fail. */
            return 0;                   /* Signs, whitespace and non-ASCII digits fail. */
        } /* End conditional branch: Condition for this action: Signs, whitespace and non-ASCII digits fail. */
        digit = (unsigned long)(*text++ - '0');                 /* Convert a known decimal digit. */
        if (value > (EXAMPLE_CONTROL_MAX_STEPS - digit) / 10UL) /* Condition for this action: Reject excess before mutation. */
        {                                                       /* Begin conditional branch: Condition for this action: Reject excess before mutation. */
            return 0;                                           /* Reject excess before mutation. */
        } /* End conditional branch: Condition for this action: Reject excess before mutation. */
        value = value * 10UL + digit; /* The result is within the tutorial's bound. */
    } /* The complete input was consumed. */
    if (!value)   /* Condition for this action: At least one successful advance is required. */
    {             /* Begin conditional branch: Condition for this action: At least one successful advance is required. */
        return 0; /* At least one successful advance is required. */
    } /* End conditional branch: Condition for this action: At least one successful advance is required. */
    *out = value; /* Publish only a valid count. */
    return 1;     /* The caller may initialize its host policy. */
} /* No VM or native platform state is opened by argument parsing. */


/* Run on the native UI thread; console input only selects the public descriptors. */
int main(int argc, char **argv)                                 /* An IDE can replace this adapter while keeping observe and the C API. */
{                                                               /* Own every descriptor and borrowed string until the synchronous run returns. */
    FVM_DesktopOptions options;                                 /* Initialized scalar host configuration. */
    FVM_DesktopSource source;                                   /* Borrow file/URL or immutable linked bytes. */
    FVM_DesktopPlaybackControl control;                         /* Separate additive command policy. */
    ExampleControl host = {0, 0, 0.0};                          /* Scalars only; no persistent managed references. */
    const char *error = 0;                                      /* Borrowed static diagnostic, never freed by this program. */
    int embedded = 0;                                           /* Select linked bytes while retaining an explicit source origin. */
    int argument;                                               /* C89 loop cursor precedes statements. */
    int valid = argc >= 2;                                      /* Require a final file/URL provenance argument. */
    int status;                                                 /* Preserve the ordinary player exit status. */
    for (argument = 1; argument < argc - 1; ++argument)         /* Translate only this executable's flags. */
    {                                                           /* Library configuration remains independent of argc/argv. */
        if (!strcmp(argv[argument], "--embedded") && !embedded) /* Condition for this action: Reject repeated selector. */
        {                                                       /* Begin conditional branch: Condition for this action: Reject repeated selector. */
            embedded = 1;                                       /* Reject repeated selector. */
        } /* End conditional branch: Condition for this action: Reject repeated selector. */
        else if (!strcmp(argv[argument], "--step") && !host.steps && argument + 1 < argc - 1) /* Consume a count. */
        {                                                                                     /* Begin conditional branch: Consume a count. */
            valid = parse_steps(argv[++argument], &host.steps) && valid;                      /* Preserve earlier parse failure. */
        } /* End conditional branch: Consume a count. */
        else           /* Unknown, repeated or incomplete arguments fail before startup. */
        {              /* Begin conditional branch: Unknown, repeated or incomplete arguments fail before startup. */
            valid = 0; /* Unknown, repeated or incomplete arguments fail before startup. */
        } /* End conditional branch: Unknown, repeated or incomplete arguments fail before startup. */
    } /* The last argument remains the source location, not another option. */
    if (!valid || argv[argc - 1][0] == '-')                                                         /* Avoid interpreting an unknown final option as a filename. */
    {                                                                                               /* Provide the thin adapter's complete command syntax. */
        fprintf(stderr, "Usage: %s [--embedded] [--step 1..10000] file-or-http[s]-URL\n", argv[0]); /* Bounded input. */
        return 2;                                                                                   /* Argument failure without opening VM/window/audio/network. */
    } /* All remaining validation belongs to the typed public API. */
    fvm_desktop_options_init(&options);                                                              /* Initialize every playback field. */
    fvm_desktop_playback_control_init(&control);                                                     /* Initialize exact ABI size and defaults. */
    options.mode = FVM_EXECUTION_INTERPRETER;                                                        /* The tutorial has no JIT capability prerequisite. */
    options.mute = 1;                                                                                /* This visual scheduling example opens no audio device. */
    options.frames = EXAMPLE_CONTROL_MAX_STEPS;                                                      /* A second bounded exit independent of commands. */
    options.stats = 1;                                                                               /* Print actual interpreter counters after playback. */
    control.initially_paused = 1;                                                                    /* READY decides when the first tick is allowed. */
    control.notify = observe;                                                                        /* Borrow the safe owner callback through runner return. */
    control.userdata = &host;                                                                        /* Stack lifetime covers every synchronous notification. */
    source.location = argv[argc - 1];                                                                /* Embedded input uses this explicit local/network origin. */
    source.kind = strstr(source.location, "://") ? FVM_DESKTOP_SOURCE_URL : FVM_DESKTOP_SOURCE_FILE; /* Select I/O. */
    if (embedded)                                                                                    /* Condition for this action: No startup file/network read in this mode. */
    {                                                                                                /* Begin conditional branch: Condition for this action: No startup file/network read in this mode. */
        source.kind = FVM_DESKTOP_SOURCE_MEMORY;                                                     /* No startup file/network read in this mode. */
    } /* End conditional branch: Condition for this action: No startup file/network read in this mode. */
    source.bytes = embedded ? embedded_movie : 0;                                                                      /* Immutable storage stays valid until return. */
    source.size = embedded ? sizeof(embedded_movie) : 0;                                                               /* Binary SWF lengths never use strlen. */
#if defined(FVM_DESKTOP_SOCKET_HTTP) || defined(FVM_DESKTOP_HTTP)                                                      /* Configured providers support child requests too. */
    {                                                                                                                  /* Keep provider policy and monitor alive for the entire run. */
        FVM_DesktopBackgroundLoader loader;                                                                            /* Worker-only byte transport, separate from VM commands. */
        FVM_DesktopLoadMonitor monitor;                                                                                /* Owner-side deadline and asynchronous delivery policy. */
        fvm_desktop_background_loader_init(&loader);                                                                   /* Explicit provider defaults, no hidden registration. */
        fvm_desktop_load_monitor_init(&monitor);                                                                       /* Default asynchronous child/form completion. */
#ifdef FVM_DESKTOP_SOCKET_HTTP                                                                                         /* Use the selected native socket adapter. */
        loader.transfer.fetch = fvm_desktop_socket_transfer_fetch;                                                     /* Default per-job socket policy. */
#else                                                                                                                  /* The configured alternate requires a worker-safe libcurl SDK. */
        loader.transfer.fetch = fvm_desktop_http_transfer_fetch; /* No VM or SDL on provider workers. */
#endif                                                                                                                 /* Provider lifetime extends through cancellation and joined cleanup. */
        loader.max_loads = EXAMPLE_CONTROL_LOADS;                                                                      /* Startup and later requests share the bound. */
        monitor.timeout_ms = EXAMPLE_CONTROL_TIMEOUT_MS;                                                               /* Cooperative cancellation, not a hard kill. */
        status = fvm_desktop_run_source_background_controlled(&options, &source, &loader, &monitor, &control, &error); /* Owner loop. */
    } /* Runner has joined workers before releasing borrowed descriptors. */
#else                                   /* Local and embedded sources remain useful without networking support. */
    if (source.kind == FVM_DESKTOP_SOURCE_URL)                                /* This executable has no configured URL transport. */
    {                                                                         /* Give the adapter's capability error before any network or window startup. */
        fprintf(stderr, "URL loading requires a configured HTTP provider\n"); /* Explicit build prerequisite. */
        status = 1;                                                           /* Ordinary capability failure; local/embedded inputs still work. */
    } /* All other sources use the same public controlled runner. */
    else                                                                                    /* Borrow until return. */
    {                                                                                       /* Begin conditional branch: Borrow until return. */
        status = fvm_desktop_run_source_controlled(&options, &source, 0, &control, &error); /* Borrow until return. */
    } /* End conditional branch: Borrow until return. */
#endif                                  /* A host can instead supply its own documented transport implementation. */
    if (status && error)                /* Condition for this action: Copy/report static text without freeing it. */
    {                                   /* Begin conditional branch: Condition for this action: Copy/report static text without freeing it. */
        fprintf(stderr, "%s\n", error); /* Copy/report static text without freeing it. */
    } /* End conditional branch: Condition for this action: Copy/report static text without freeing it. */
    printf("completed=%lu\n", host.completed); /* Inspect only a copied scalar after runner return. */
    fvm_desktop_shutdown();                    /* Player objects and workers are already gone; release SDK session state. */
    return status;                             /* No worker, borrowed snapshot or native audio state remains. */
} /* The host may now release its own source/provider/userdata storage. */
