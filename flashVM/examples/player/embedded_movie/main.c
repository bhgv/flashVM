/* Embedded SWF playback; see README.md / README.ru.md for ownership and builds. */
#include "../../../frontend/common/player.h" /* Shared C89 playback API. */
#include "movie.h" /* Linked read-only SWF, no installed asset file. */
#define EXAMPLE_EMBEDDED_TICKS 2UL /* Bound automated playback. */
#define EXAMPLE_EMBEDDED_FPS 1000.0 /* Fast scheduling within the public range. */

/* Run from the native UI thread; no console arguments are needed. */
int main(void) /* Can also be called from an application-owned launcher. */
{ /* Begin caller ownership. */
    FVM_DesktopOptions options; /* Scalar settings; no file playlist is required. */
    int result; /* Preserve the shared runner's exit status. */
    fvm_desktop_options_init(&options); /* Initialize every field. */
    options.mode = FVM_EXECUTION_INTERPRETER; /* This example does not require JIT. */
    options.mute = 1; /* Root stream controls are explicitly muted. */
    options.frames = EXAMPLE_EMBEDDED_TICKS; /* Return after two timeline ticks. */
    options.fps = EXAMPLE_EMBEDDED_FPS; /* Keep percent at the default 100. */
    result = fvm_desktop_run_memory(&options, embedded_movie, sizeof(embedded_movie), /* Exact bytes. */
                                   "embedded://startup.swf", 0); /* Label, not a disk path. */
    fvm_desktop_shutdown(); /* End the matching library session after all player resources. */
    return result; /* No managed/native resource escapes the synchronous runner. */
} /* End host ownership; the immutable SWF remains part of the executable. */
