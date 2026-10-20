/* Shared module containing its own linked SWF; see README.md / README.ru.md. */
#include "module.h" /* Export the C89/C++98 module interface. */
#include "../../../frontend/common/player.h" /* Borrowed-buffer runner. */
#include "movie.h" /* Binary resource is part of this shared library. */
#define EXAMPLE_MODULE_TICKS 2UL /* Bound the module's synchronous preview. */
#define EXAMPLE_MODULE_FPS 1000.0 /* Fast scheduling, within the public limit. */

/* The caller owns final global SDL shutdown and the UI thread. */
int embedded_movie_play(void) /* No CLI, filename or application-specific struct. */
{ /* Acquire only temporary player resources. */
    FVM_DesktopOptions options; /* Caller-local options, never retained. */
    fvm_desktop_options_init(&options); /* Shared defaults initialize every field. */
    options.mode = FVM_EXECUTION_INTERPRETER; /* Portable mode without JIT requirements. */
    options.mute = 1; /* Never open audio for this resource. */
    options.frames = EXAMPLE_MODULE_TICKS; /* Return without user interaction. */
    options.fps = EXAMPLE_MODULE_FPS; /* Keep authored percentage at 100. */
    return fvm_desktop_run_memory(&options, embedded_movie, sizeof(embedded_movie), /* Exact binary. */
                                  "embedded://module.swf", 0); /* No runtime asset lookup. */
} /* Player releases VM roots/window; the module retains its immutable movie bytes. */
