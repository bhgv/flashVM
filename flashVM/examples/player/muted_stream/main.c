/* Explicitly import a root stream movie without sound; no SDL/device dependency. */
#include "flashvm/movieclip.h" /* Strict and flagged import plus timeline advance. */
#include "flashvm/standard.h" /* Install standard MovieClip prototypes before import. */
#include "flashvm/swf_stream_sound.h" /* Inspect the original source's retained stream count. */
#include "flashvm/gc.h" /* Pin the returned clip/library across host collection. */
#include <stdio.h> /* Host-owned regular-file input, diagnostics and result output. */
#include <stdlib.h> /* Host-owned temporary input storage. */
#include "bounds.h" /* Explicit example-owned bound; no private VM implementation include. */

/* Translate status locally; no library operation prints or exits on our behalf. */
static int ok(FVM_Result result, const char *operation) /* Scalar status, no borrowed VM value. */
{ /* Begin diagnostic helper. */
    if (result == FVM_OK) return 1; /* Normal progress. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Host-selected reporting. */
    return 0; /* Caller still owns every acquired resource. */
} /* End diagnostic helper. */

/* The ordinary VM CMake example target links only the public C89 ABI. */
int main(int argc, char **argv) /* Give the complete SWF path as the only argument. */
{ /* Initialize ownership before any fallible acquisition. */
    FILE *input = NULL; /* Owned stream, closed on every path. */
    unsigned char *bytes = NULL; /* Own input through the import call. */
    FVM_Config config; /* Host-owned runtime configuration. */
    FVM_Runtime *runtime = NULL; /* Destroy after its context. */
    FVM_Context *context = NULL; /* Own source bytes, prototypes and managed objects. */
    FVM_Object *clip = NULL; /* [GC:] Borrowed managed result, explicitly pinned below. */
    long length; /* Check signed ftell before size_t conversion. */
    size_t streams = 0; /* Copied scalar metadata, no lifetime restriction. */
    int status = 1, pinned = 0, complete; /* Track cleanup and host read status. */
    if (argc != 2) { fprintf(stderr, "usage: muted_stream FILE.swf\n"); return 2; } /* No file yet. */
    input = fopen(argv[1], "rb"); /* Preserve raw bytes on Windows too. */
    if (!input) goto cleanup; /* No partial input is accepted. */
    if (fseek(input, 0, SEEK_END) || (length = ftell(input)) <= 0 || /* Require seekable input. */
        length > FVM_EXAMPLE_MUTED_STREAM_INPUT_LIMIT || fseek(input, 0, SEEK_SET)) goto cleanup; /* Bound allocation. */
    bytes = (unsigned char *)malloc((size_t)length); /* Allocate the exact measured size. */
    if (!bytes) goto cleanup; /* Stream cleanup is still required. */
    complete = fread(bytes, 1, (size_t)length, input) == (size_t)length; /* Reject truncation. */
    if (complete && fgetc(input) != EOF) complete = 0; /* Detect growth beyond the measured end. */
    if (ferror(input)) complete = 0; /* Distinguish read failure from clean EOF. */
    if (fclose(input)) complete = 0; /* Close even when reading failed. */
    input = NULL; /* Prevent double close in shared cleanup. */
    if (!complete) goto cleanup; /* Never import an incomplete or growing prefix. */
    if (!ok(fvm_config_init(&config), "config")) goto cleanup; /* Initialize the ABI structure. */
    if (!ok(fvm_runtime_create(&config, &runtime), "runtime")) goto cleanup; /* Own runtime. */
    if (!ok(fvm_context_create(runtime, &context), "context")) goto cleanup; /* Own one VM context. */
    if (!ok(fvm_load_memory(context, bytes, (size_t)length), "load")) goto cleanup; /* Copy source. */
    if (!ok(fvm_standard_install(context), "standard")) goto cleanup; /* Prepare MovieClip model. */
    if (!ok(fvm_swf_stream_sound_count(context, &streams), "stream count")) goto cleanup; /* Validate root. */
    if (!ok(fvm_movieclip_load_swf_ex(context, bytes, (size_t)length, "_root", /* Same exact source. */
             FVM_SWF_IMPORT_MUTE_ROOT_STREAMS, &clip), "muted scene import")) goto cleanup; /* Explicit opt-in. */
    if (!ok(fvm_gc_pin(context, clip), "pin")) goto cleanup; /* [GC:] Retain clip and its library graph. */
    pinned = 1; /* [GC:] Cleanup now owns one matching unpin. */
    free(bytes); /* Source and scene own all retained bytes independently. */
    bytes = NULL; /* Prevent double release in the shared cleanup path. */
    if (!ok(fvm_gc_step(context, 0), "GC")) goto cleanup; /* [GC:] Safe host boundary outside callbacks. */
    if (!ok(fvm_movieclip_advance(context, clip, 1), "advance")) goto cleanup; /* No audio device is opened. */
    if (printf("streams=%lu advanced=1 muted=1\n", (unsigned long)streams) < 0 || fflush(stdout)) goto cleanup; /* Explicit C89 formatting. */
    status = 0; /* Complete host operation; release resources below. */
cleanup: /* Reverse acquisition order on both success and failure. */
    if (status) fprintf(stderr, "input/import did not complete\n"); /* Include early host I/O errors. */
    if (pinned) fvm_gc_unpin(context, clip); /* [GC:] Release only a successfully acquired root. */
    if (context) fvm_context_destroy(context); /* Reclaim the source and managed graph. */
    if (runtime) fvm_runtime_destroy(runtime); /* No context may outlive its runtime. */
    if (input) fclose(input); /* Handle failure before the normal input close. */
    free(bytes); /* NULL is accepted after successful ownership transfer. */
    return status; /* 0 success, 1 operation/I/O failure, 2 usage. */
} /* End standalone host tutorial. */
