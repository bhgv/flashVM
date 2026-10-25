/* Observe network loads on the C host thread and optionally stop before startup I/O. */
#include "player.h"            /* Public C89 host, monitor and policy declarations. */
#include <stdio.h>             /* Print bounded diagnostic metadata outside the worker. */
#include <string.h>            /* Compare the optional example-only stop selector. */
#ifdef FVM_DESKTOP_SOCKET_HTTP /* The native provider is selected by the SDK build. */
#include "socket_http.h"       /* Native worker adapter; null userdata selects defaults. */
#else                          /* An alternative SDK may provide libcurl. */
#include "http.h"              /* Thread-safe libcurl worker adapter. */
#endif                         /* Finish provider-specific declarations. */
/* The caller owns this state until the synchronous whole-player call returns. */
typedef struct Progress /* Observer state never enters a network worker. */
{                       /* Begin caller-owned fields. */
    int stop_startup;   /* Nonzero asks the BEGIN notice to stop without network I/O. */
    unsigned long id;   /* Last displayed request ordinal. */
    size_t received;    /* Last displayed body-byte progress, capped by the loader budget. */
} Progress;             /* End the private example policy. */


/* Runner invokes this function on its owner thread, including before an SDL window exists. */
static int progress(const FVM_DesktopLoadEvent *event, void *userdata) /* Borrow one notice. */
{                                                                      /* Begin observer callback. */
    Progress *state = (Progress *)userdata;                            /* Recover caller-owned immutable policy/state. */
    if (event->phase != FVM_DESKTOP_LOAD_WAIT ||                       /* Always show lifecycle transitions. */
        event->id != state->id ||                                      /* A new request starts its own byte count. */
        event->received != state->received)                            /* Suppress unchanged WAIT heartbeats. */
    {                                                                  /* Print new progress or a lifecycle transition. */
        printf("load=%lu kind=%u phase=%u received=%lu url=%s\n",      /* Metadata only. */
               event->id, event->kind, event->phase,                   /* Copy scalar request metadata. */
               (unsigned long)event->received, event->reference);      /* Borrow URL only here. */
    } /* Finish the diagnostic branch. */
    state->id = event->id;                         /* Remember this request for progress coalescing. */
    state->received = event->received;             /* Copy only scalar metadata; strings remain borrowed. */
    return state->stop_startup &&                  /* Caller opted into a startup stop. */
           event->phase == FVM_DESKTOP_LOAD_BEGIN; /* No provider starts for this request. */
} /* Return to owner loop; no VM calls, worker joins or reentrant runner calls here. */


/* Link this function with the static host target or its shared-library equivalent. */
int main(int argc, char **argv)                                         /* Ordinary application entrypoint, independent of player CLI. */
{                                                                       /* Begin stack-owned configuration. */
    FVM_DesktopOptions options;                                         /* Playback settings initialized below. */
    FVM_DesktopBackgroundLoader loader;                                 /* Worker provider and byte/resource budgets. */
    FVM_DesktopLoadMonitor monitor;                                     /* Owner observation, async mode and total job deadline. */
    FVM_DesktopSource source;                                           /* One borrowed URL supplied by the calling application. */
    Progress state = {0, 0, 0};                                         /* No pointer or global registration is needed. */
    const char *error = 0;                                              /* Runner returns an optional borrowed static diagnostic. */
    int status;                                                         /* Preserve successful stop versus playback failure. */
    if (argc < 2 || argc > 3 ||                                         /* Require one URL and at most one selector. */
        (argc == 3 && strcmp(argv[2], "--cancel-startup")))             /* Reject unknown selectors. */
    {                                                                   /* Reject malformed calls before SDK/network startup. */
        fprintf(stderr, "usage: %s URL [--cancel-startup]\n", argv[0]); /* Example-specific help. */
        return 2;                                                       /* Ordinary argument-error status. */
    } /* Continue with a validated source and optional stop request. */
    if (!fvm_desktop_background_supported())                /* Check linked default SDK worker capability. */
    {                                                       /* Older curl SDKs or provider-less builds cannot run this example. */
        fputs("background provider unavailable\n", stderr); /* Explicitly report unavailable support. */
        return 1;                                           /* Preserve a real unsupported outcome. */
    } /* Initialize every public descriptor before editing fields. */
    fvm_desktop_options_init(&options);                                                      /* No hidden console parser is involved. */
    fvm_desktop_background_loader_init(&loader);                                             /* Initialize native fetch defaults. */
    fvm_desktop_load_monitor_init(&monitor);                                                 /* Async mode, unlimited deadline and no callback. */
#ifdef FVM_DESKTOP_SOCKET_HTTP                                                               /* Select exactly the provider linked into this SDK. */
    loader.transfer.fetch = fvm_desktop_socket_transfer_fetch;                               /* Null provider userdata uses defaults. */
#else                                                                                        /* The other supported worker adapter is libcurl. */
    loader.transfer.fetch = fvm_desktop_http_transfer_fetch; /* Rejects unsafe old global APIs. */
#endif                                                                                       /* Provider ownership stays with the calling application. */
    options.frames = 700;                                                                    /* Bound this fast loopback assertion tutorial. */
    options.fps = 1000;                                                                      /* Exercise continuing frames/timers while resources arrive. */
    options.mute = 1;                                                                        /* No audio device is needed for these silent example movies. */
    loader.max_loads = 8;                                                                    /* Startup plus child/form requests share this budget. */
    monitor.timeout_ms = 5000;                                                               /* Total per-network-job budget, with cooperative final join. */
    monitor.notify = progress;                                                               /* Callback stays on the owner thread. */
    monitor.userdata = &state;                                                               /* Stack state remains live through the entire call. */
    state.stop_startup = argc == 3;                                                          /* Demonstrate cancelling before any provider is launched. */
    source.kind = FVM_DESKTOP_SOURCE_URL;                                                    /* Exact URL source kind. */
    source.location = argv[1];                                                               /* Borrow caller text through runner return. */
    source.bytes = 0;                                                                        /* URL sources have no borrowed memory payload. */
    source.size = 0;                                                                         /* URL byte size is discovered by the native job. */
    status = fvm_desktop_run_source_monitored(                                               /* Enter the synchronous owner playback loop. */
                                              &options, &source, &loader, &monitor, &error); /* Borrow all descriptors until return. */
    if (error)                                                                               /* Condition for this action: Static error remains borrowed; never free it. */
    {                                                                                        /* Begin conditional branch: Condition for this action: Static error remains borrowed; never free it. */
        fprintf(stderr, "%s\n", error);                                                      /* Static error remains borrowed; never free it. */
    } /* End conditional branch: Condition for this action: Static error remains borrowed; never free it. */
    if (fvm_desktop_shutdown() != FVM_OK) /* Condition for this action: Jobs have joined before SDK cleanup. */
    {                                     /* Begin conditional branch: Condition for this action: Jobs have joined before SDK cleanup. */
        return 1;                         /* Jobs have joined before SDK cleanup. */
    } /* End conditional branch: Condition for this action: Jobs have joined before SDK cleanup. */
    return status; /* Stop at BEGIN returns zero; timeout/import failure remains nonzero. */
} /* All caller-owned descriptors and strings may now be released. */
