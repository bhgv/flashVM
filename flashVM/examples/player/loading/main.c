/* Embedded startup with child/form loading; see README.md / README.ru.md. */
#include "../../../frontend/common/player.h" /* Shared C89 runner and scalar options. */
#include "startup.h" /* Startup movie is linked, not opened as a file. */
#define EXAMPLE_LOADING_REQUESTS 8UL /* Include startup, child and both variable requests. */
#define EXAMPLE_LOADING_FRAMES 3UL /* The last frame asserts the loaded values. */
#define EXAMPLE_LOADING_FPS 30.0 /* Keep the preview visible at a normal cadence. */

/* An optional absolute source name supplies provenance when OS path discovery is unavailable. */
int main(int argc, char **argv) /* No argument means the executable directory is the local base. */
{ /* Begin caller-owned configuration. */
    FVM_DesktopOptions options; /* No file playlist is required. */
    int status; /* Preserve playback/import/argument status. */
    fvm_desktop_options_init(&options); /* Initialize every field using shared defaults. */
    options.mode = FVM_EXECUTION_INTERPRETER; /* Independent of JIT availability. */
    options.mute = 1; /* This tutorial does not open an audio endpoint. */
    options.max_loads = EXAMPLE_LOADING_REQUESTS; /* Explicitly permit deferred resource requests. */
    options.frames = EXAMPLE_LOADING_FRAMES; /* Finish without waiting for user input. */
    options.fps = EXAMPLE_LOADING_FPS; /* Leave percentage at its default 100. */
    status = fvm_desktop_run_memory(&options, loading_startup, sizeof(loading_startup), /* Binary. */
                                    argc > 1 ? argv[1] : "embedded://loading.swf", 0); /* Origin. */
    fvm_desktop_shutdown(); /* End the matching library session after all player resources. */
    return status; /* Startup, child and decoded variables were checked by the movie itself. */
} /* All temporary VM objects, pins and native resources have been released. */
