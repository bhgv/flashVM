/* Direct structured playback tutorial; see README.md and README.ru.md for the full contract. */
#include "../../../frontend/common/player.h" /* C89 host boundary, options and scalar statuses. */
#include <stdio.h>                           /* The embedding host selects where to report its static error text. */
#define EXAMPLE_CONFIGURED_FRAMES 2UL        /* Two timeline ticks keep automated runs bounded. */
#define EXAMPLE_CONFIGURED_FPS 1000.0        /* Fast deterministic scheduling, within the player limit. */


/* Run on the native main/UI thread; the runner owns and cleans up its temporary VM. */
int main(int argc, char **argv)           /* Supply exactly one SWF path, without player CLI options. */
{                                         /* Begin the synchronous embedding session. */
    FVM_DesktopOptions options;           /* Caller-owned settings, initialized before use. */
    const char *paths[1];                 /* Borrow the process argument string through playback return. */
    const char *error = 0;                /* Static diagnostic text; never free it. */
    int result;                           /* Preserve the runner's documented exit status. */
    if (argc != 2)                        /* Require exactly one movie path. */
    {                                     /* Reject usage before acquiring anything. */
        return FVM_DESKTOP_EXIT_ARGUMENT; /* Preserve the documented argument status. */
    } /* End usage validation. */
    if (!fvm_desktop_options_init(&options)) /* Use the same defaults as the CLI adapter. */
    {                                        /* Initialization accepts required caller storage. */
        return FVM_DESKTOP_EXIT_ARGUMENT;    /* No resources exist to release here. */
    } /* End default initialization. */
    paths[0] = argv[1];                         /* The caller retains immutable path storage. */
    options.files = paths;                      /* The runner borrows the complete playlist array. */
    options.file_count = 1;                     /* Exactly one initialized entry is accessible. */
    options.mode = FVM_EXECUTION_INTERPRETER;   /* This tutorial does not require JIT availability. */
    options.mute = 1;                           /* Explicitly avoid opening any audio device. */
    options.frames = EXAMPLE_CONFIGURED_FRAMES; /* Stop without requiring user input. */
    options.fps = EXAMPLE_CONFIGURED_FPS;       /* Leave the default percentage unchanged. */
    result = fvm_desktop_run(&options, &error); /* Validate all fields, load, play and release. */
    fvm_desktop_shutdown();                     /* End the matching library session after all player resources. */
    if (error)                                  /* The runner returns borrowed static text only on failure. */
    {                                           /* Select the diagnostic sink in application code. */
        fprintf(stderr, "%s\n", error);         /* Never free this message. */
    } /* End host diagnostic handling. */
    return result; /* No borrowed path, VM object or native window escapes the call. */
} /* End the embedding session; the application owns final global SDL shutdown if needed. */
