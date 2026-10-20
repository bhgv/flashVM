/* Opt-in background provider with native event pumping; see README.md / README.ru.md. */
#include "../../../frontend/common/player.h" /* Public options, source and background C APIs. */
#ifdef FVM_DESKTOP_SOCKET_HTTP /* This target selects exactly one configured transport. */
#include "../../../frontend/common/socket_http.h" /* Native worker-safe provider adapter. */
#else /* The alternate build uses the configured libcurl provider. */
#include "../../../frontend/common/http.h" /* Requires libcurl reporting thread-safe global APIs. */
#endif /* Neither backend is silently substituted for a custom host callback. */
#include <stdio.h> /* Report a static player error without taking ownership. */
#define EXAMPLE_BACKGROUND_LOADS 8UL /* Startup and all child/form requests share this budget. */
#define EXAMPLE_BACKGROUND_FRAMES 3UL /* The loading tutorial validates values on its last tick. */

/* Run on the native UI thread; only resource bytes are fetched on joined workers. */
int main(int argc, char **argv) /* One absolute HTTP(S) URL selects the startup movie. */
{ /* Own all descriptors until the synchronous player call returns. */
    FVM_DesktopOptions options; /* Scalar playback configuration, initialized below. */
    FVM_DesktopBackgroundLoader loader; /* Explicit worker policy, independent of CLI parsing. */
    FVM_DesktopSource source; /* Borrow the URL from argv for this call. */
    const char *error = 0; /* Optional error points to static library text. */
    int status; /* Preserve the ordinary 0/1/2 player status for the caller. */
    if (argc != 2) /* Reject missing or ambiguous source before any native startup. */
    { /* Show this embedding example's own small argument interface. */
        fprintf(stderr, "Usage: %s http[s]://host/path/start.swf\n", argv[0]); /* Required URL. */
        return 2; /* Argument error, matching the player convention. */
    } /* Complete argument shape; URI validation belongs to the provider. */
    fvm_desktop_options_init(&options); /* No caller field remains uninitialized. */
    fvm_desktop_background_loader_init(&loader); /* No hidden provider registration. */
#ifdef FVM_DESKTOP_SOCKET_HTTP /* Select the native TCP/TLS worker adapter. */
    loader.transfer.fetch = fvm_desktop_socket_transfer_fetch; /* Null userdata uses defaults. */
#else /* The optional libcurl implementation performs its SDK thread-safety check. */
    loader.transfer.fetch = fvm_desktop_http_transfer_fetch; /* Unsupported older SDKs fail. */
#endif /* Only this per-run policy changes, not a global callback slot. */
    loader.max_loads = EXAMPLE_BACKGROUND_LOADS; /* Permit child movie and form requests. */
    options.mode = FVM_EXECUTION_INTERPRETER; /* No JIT dependency in this tutorial. */
    options.mute = 1; /* No audio device is opened. */
    options.frames = EXAMPLE_BACKGROUND_FRAMES; /* Finish after the tutorial's assertions. */
    source.kind = FVM_DESKTOP_SOURCE_URL; /* Fetch startup and preserve canonical provenance. */
    source.bytes = 0; /* No borrowed memory movie is supplied in this example. */
    source.size = 0; /* The provider determines each counted resource length. */
    source.location = argv[1]; /* Borrowed URL lives through synchronous return. */
    status = fvm_desktop_run_source_background(&options, &source, &loader, &error); /* Owner loop. */
    if (status && error) fprintf(stderr, "%s\n", error); /* Never free the static explanation. */
    fvm_desktop_shutdown(); /* All resource workers have joined before SDK session cleanup. */
    return status; /* No worker, GC pin or temporary resource remains. */
} /* The native host can now dispose its own borrowed provider state. */
