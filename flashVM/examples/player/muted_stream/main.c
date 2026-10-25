/* Explicitly import a root stream movie without sound; no SDL/device dependency. */
#include "flashvm/movieclip.h"        /* Strict and flagged import plus timeline advance. */
#include "flashvm/standard.h"         /* Install standard MovieClip prototypes before import. */
#include "flashvm/swf_stream_sound.h" /* Inspect the original source's retained stream count. */
#include "flashvm/gc.h"               /* Pin the returned clip/library across host collection. */
#include <stdio.h>                    /* Host-owned regular-file input, diagnostics and result output. */
#include <stdlib.h>                   /* Host-owned temporary input storage. */
#include "bounds.h"                   /* Explicit example-owned bound; no private VM implementation include. */


/* Translate status locally; no library operation prints or exits on our behalf. */
static int ok(FVM_Result result, const char *operation) /* Scalar status, no borrowed VM value. */
{                                                       /* Begin diagnostic helper. */
    if (result == FVM_OK)                               /* Condition for this action: Normal progress. */
    {                                                   /* Begin conditional branch: Condition for this action: Normal progress. */
        return 1;                                       /* Normal progress. */
    } /* End conditional branch: Condition for this action: Normal progress. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Host-selected reporting. */
    return 0;                                                   /* Caller still owns every acquired resource. */
} /* End diagnostic helper. */


/* The ordinary VM CMake example target links only the public C89 ABI. */
int main(int argc, char **argv)                            /* Give the complete SWF path as the only argument. */
{                                                          /* Initialize ownership before any fallible acquisition. */
    FILE *input = NULL;                                    /* Owned stream, closed on every path. */
    unsigned char *bytes = NULL;                           /* Own input through the import call. */
    FVM_Config config;                                     /* Host-owned runtime configuration. */
    FVM_Runtime *runtime = NULL;                           /* Destroy after its context. */
    FVM_Context *context = NULL;                           /* Own source bytes, prototypes and managed objects. */
    FVM_Object *clip = NULL;                               /* [GC:] Borrowed managed result, explicitly pinned below. */
    long length;                                           /* Check signed ftell before size_t conversion. */
    size_t streams = 0;                                    /* Copied scalar metadata, no lifetime restriction. */
    int status = 1, pinned = 0, complete;                  /* Track cleanup and host read status. */
    if (argc != 2)                                         /* Condition for this action: No file yet. */
    {                                                      /* Begin conditional branch: Condition for this action: No file yet. */
        fprintf(stderr, "usage: muted_stream FILE.swf\n"); /* No file yet. */
        return 2;                                          /* No file yet. */
    } /* No file yet. */
    input = fopen(argv[1], "rb"); /* Preserve raw bytes on Windows too. */
    if (!input)                   /* Condition for this action: No partial input is accepted. */
    {                             /* Begin conditional branch: Condition for this action: No partial input is accepted. */
        goto cleanup;             /* No partial input is accepted. */
    } /* End conditional branch: Condition for this action: No partial input is accepted. */
    if (fseek(input, 0, SEEK_END) || (length = ftell(input)) <= 0 ||                /* Require seekable input. */
        length > FVM_EXAMPLE_MUTED_STREAM_INPUT_LIMIT || fseek(input, 0, SEEK_SET)) /* Bound allocation. */
    {                                                                               /* Begin scope: Bound allocation. */
        goto cleanup;                                                               /* Bound allocation. */
    } /* End scope: Bound allocation. */
    bytes = (unsigned char *)malloc((size_t)length); /* Allocate the exact measured size. */
    if (!bytes)                                      /* Condition for this action: Stream cleanup is still required. */
    {                                                /* Begin conditional branch: Condition for this action: Stream cleanup is still required. */
        goto cleanup;                                /* Stream cleanup is still required. */
    } /* End conditional branch: Condition for this action: Stream cleanup is still required. */
    complete = fread(bytes, 1, (size_t)length, input) == (size_t)length; /* Reject truncation. */
    if (complete && fgetc(input) != EOF)                                 /* Condition for this action: Detect growth beyond the measured end. */
    {                                                                    /* Begin conditional branch: Condition for this action: Detect growth beyond the measured end. */
        complete = 0;                                                    /* Detect growth beyond the measured end. */
    } /* End conditional branch: Condition for this action: Detect growth beyond the measured end. */
    if (ferror(input)) /* Condition for this action: Distinguish read failure from clean EOF. */
    {                  /* Begin conditional branch: Condition for this action: Distinguish read failure from clean EOF. */
        complete = 0;  /* Distinguish read failure from clean EOF. */
    } /* End conditional branch: Condition for this action: Distinguish read failure from clean EOF. */
    if (fclose(input)) /* Condition for this action: Close even when reading failed. */
    {                  /* Begin conditional branch: Condition for this action: Close even when reading failed. */
        complete = 0;  /* Close even when reading failed. */
    } /* End conditional branch: Condition for this action: Close even when reading failed. */
    input = NULL;     /* Prevent double close in shared cleanup. */
    if (!complete)    /* Condition for this action: Never import an incomplete or growing prefix. */
    {                 /* Begin conditional branch: Condition for this action: Never import an incomplete or growing prefix. */
        goto cleanup; /* Never import an incomplete or growing prefix. */
    } /* End conditional branch: Condition for this action: Never import an incomplete or growing prefix. */
    if (!ok(fvm_config_init(&config), "config")) /* Condition for this action: Initialize the ABI structure. */
    {                                            /* Begin conditional branch: Condition for this action: Initialize the ABI structure. */
        goto cleanup;                            /* Initialize the ABI structure. */
    } /* End conditional branch: Condition for this action: Initialize the ABI structure. */
    if (!ok(fvm_runtime_create(&config, &runtime), "runtime")) /* Condition for this action: Own runtime. */
    {                                                          /* Begin conditional branch: Condition for this action: Own runtime. */
        goto cleanup;                                          /* Own runtime. */
    } /* End conditional branch: Condition for this action: Own runtime. */
    if (!ok(fvm_context_create(runtime, &context), "context")) /* Condition for this action: Own one VM context. */
    {                                                          /* Begin conditional branch: Condition for this action: Own one VM context. */
        goto cleanup;                                          /* Own one VM context. */
    } /* End conditional branch: Condition for this action: Own one VM context. */
    if (!ok(fvm_load_memory(context, bytes, (size_t)length), "load")) /* Condition for this action: Copy source. */
    {                                                                 /* Begin conditional branch: Condition for this action: Copy source. */
        goto cleanup;                                                 /* Copy source. */
    } /* End conditional branch: Condition for this action: Copy source. */
    if (!ok(fvm_standard_install(context), "standard")) /* Condition for this action: Prepare MovieClip model. */
    {                                                   /* Begin conditional branch: Condition for this action: Prepare MovieClip model. */
        goto cleanup;                                   /* Prepare MovieClip model. */
    } /* End conditional branch: Condition for this action: Prepare MovieClip model. */
    if (!ok(fvm_swf_stream_sound_count(context, &streams), "stream count")) /* Condition for this action: Validate root. */
    {                                                                       /* Begin conditional branch: Condition for this action: Validate root. */
        goto cleanup;                                                       /* Validate root. */
    } /* End conditional branch: Condition for this action: Validate root. */
    if (!ok(fvm_movieclip_load_swf_ex(context, bytes, (size_t)length, "_root",  /* Same exact source. */
                                      FVM_SWF_IMPORT_MUTE_ROOT_STREAMS, &clip), /* Explicit opt-in. */
            "muted scene import"))                                              /* Explicit opt-in. */
    {                                                                           /* Begin scope: Explicit opt-in. */
        goto cleanup;                                                           /* Explicit opt-in. */
    } /* End scope: Explicit opt-in. */
    if (!ok(fvm_gc_pin(context, clip), "pin")) /* Condition for this action: [GC:] Retain clip and its library graph. */
    {                                          /* Begin conditional branch: Condition for this action: [GC:] Retain clip and its library graph. */
        goto cleanup;                          /* [GC:] Retain clip and its library graph. */
    } /* End conditional branch: Condition for this action: [GC:] Retain clip and its library graph. */
    pinned = 1;                             /* [GC:] Cleanup now owns one matching unpin. */
    free(bytes);                            /* Source and scene own all retained bytes independently. */
    bytes = NULL;                           /* Prevent double release in the shared cleanup path. */
    if (!ok(fvm_gc_step(context, 0), "GC")) /* Condition for this action: [GC:] Safe host boundary outside callbacks. */
    {                                       /* Begin conditional branch: Condition for this action: [GC:] Safe host boundary outside callbacks. */
        goto cleanup;                       /* [GC:] Safe host boundary outside callbacks. */
    } /* End conditional branch: Condition for this action: [GC:] Safe host boundary outside callbacks. */
    if (!ok(fvm_movieclip_advance(context, clip, 1), "advance")) /* Condition for this action: No audio device is opened. */
    {                                                            /* Begin conditional branch: Condition for this action: No audio device is opened. */
        goto cleanup;                                            /* No audio device is opened. */
    } /* End conditional branch: Condition for this action: No audio device is opened. */
    if (printf("streams=%lu advanced=1 muted=1\n", (unsigned long)streams) < 0 || fflush(stdout)) /* Condition for this action: Explicit C89 formatting. */
    {                                                                                             /* Begin conditional branch: Condition for this action: Explicit C89 formatting. */
        goto cleanup;                                                                             /* Explicit C89 formatting. */
    } /* End conditional branch: Condition for this action: Explicit C89 formatting. */
    status = 0;                                             /* Complete host operation; release resources below. */
cleanup:                                                    /* Reverse acquisition order on both success and failure. */
    if (status)                                             /* Condition for this action: Include early host I/O errors. */
    {                                                       /* Begin conditional branch: Condition for this action: Include early host I/O errors. */
        fprintf(stderr, "input/import did not complete\n"); /* Include early host I/O errors. */
    } /* End conditional branch: Condition for this action: Include early host I/O errors. */
    if (pinned)                      /* Condition for this action: [GC:] Release only a successfully acquired root. */
    {                                /* Begin conditional branch: Condition for this action: [GC:] Release only a successfully acquired root. */
        fvm_gc_unpin(context, clip); /* [GC:] Release only a successfully acquired root. */
    } /* End conditional branch: Condition for this action: [GC:] Release only a successfully acquired root. */
    if (context)                      /* Condition for this action: Reclaim the source and managed graph. */
    {                                 /* Begin conditional branch: Condition for this action: Reclaim the source and managed graph. */
        fvm_context_destroy(context); /* Reclaim the source and managed graph. */
    } /* End conditional branch: Condition for this action: Reclaim the source and managed graph. */
    if (runtime)                      /* Condition for this action: No context may outlive its runtime. */
    {                                 /* Begin conditional branch: Condition for this action: No context may outlive its runtime. */
        fvm_runtime_destroy(runtime); /* No context may outlive its runtime. */
    } /* End conditional branch: Condition for this action: No context may outlive its runtime. */
    if (input)         /* Condition for this action: Handle failure before the normal input close. */
    {                  /* Begin conditional branch: Condition for this action: Handle failure before the normal input close. */
        fclose(input); /* Handle failure before the normal input close. */
    } /* End conditional branch: Condition for this action: Handle failure before the normal input close. */
    free(bytes);   /* NULL is accepted after successful ownership transfer. */
    return status; /* 0 success, 1 operation/I/O failure, 2 usage. */
} /* End standalone host tutorial. */
