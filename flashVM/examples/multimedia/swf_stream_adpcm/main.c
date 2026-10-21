/* Tutorial: prepare counted partial ADPCM, then export native S16 after destroying the VM. */
#include "flashvm/swf_stream_pcm.h" /* Declare counted preparation and independent native reads. */
#include <string.h> /* Validate the explicit allocation-free range export option. */
#include <stdio.h> /* Open only the explicitly requested native-S16 output file. */
#include "example_movie.h" /* Own the SWF bytes, authoritative counts and bounded geometry. */
/* Translate result codes into ordinary tutorial error handling; no assertion aborts. */
static int checked(FVM_Result result, const char *operation) /* Keep the failed API name. */
{ /* Begin recoverable status translation. */
    if (result == FVM_OK) return 1; /* Continue after a complete acknowledged operation. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Preserve the exact error. */
    return 0; /* Ask the caller to release partial ownership. */
} /* Finish status translation. */
/* Retain a native snapshot until every reader and file write has finished. */
int main(int argc, char **argv) /* One optional output path; no hidden input or timing policy. */
{ /* Begin the owner-side tutorial. */
    FVM_Config config; /* Runtime configuration is copied during creation. */
    FVM_Runtime *runtime = 0; /* Owner releases this after preparing native PCM. */
    FVM_Context *context = 0; /* Source inspection stays on this context's owner thread. */
    FVM_SWFStreamSoundSnapshot *source = 0; /* Independent native snapshot survives VM teardown. */
    FVM_SWFStreamPCMReader reader; /* Borrow source until the copy loop has stopped. */
    FVM_SWFStreamPCMChunk chunk; /* Receive exact frame metadata for each copied range. */
    FILE *file = 0; /* Caller-requested file, closed on success and every error path. */
    short pcm[EXAMPLE_ADPCM_CHUNK]; /* Caller-owned aligned mono S16 transfer buffer. */
    size_t written = 0, total = 0; /* Count scalar values and accepted mono source frames. */
    FVM_Result result = FVM_OK; /* Retain the exact read status at EOF or failure. */
    int ranges = 0; /* Choose encoded native ranges instead of eager normalized PCM. */
    int status = 1; /* Default to failure until export and close both succeed. */
    if (argc == 1) /* Usage has no file or VM side effects. */
    { /* Explain the one explicit export action. */
        puts("Usage: swf_stream_adpcm OUTPUT.pcm [--ranges] (native S16 mono 44100 Hz)"); /* No output file. */
        return 0; /* A usage request succeeds. */
    } /* End the no-side-effect usage path. */
    if (argc == 3 && strcmp(argv[2], "--ranges") == 0) ranges = 1; /* Explicit raw-read policy. */
    else if (argc != 2) return 1; /* Reject malformed command lines before allocating or opening files. */
    if (!checked(fvm_config_init(&config), "config")) goto cleanup; /* Initialize owner policy. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime")) goto cleanup; /* Acquire VM. */
    if (!checked(fvm_context_create(runtime, &context), "context")) goto cleanup; /* Acquire source. */
    if (!checked(fvm_load_memory(context, movie, sizeof(movie)), "load")) goto cleanup; /* Real SWF8. */
    if (ranges) result = fvm_swf_stream_sound_snapshot_open_bounded( /* Keep compressed ownership. */
        context, 0, EXAMPLE_ADPCM_BUDGET, &source); /* Copy without decoding; caller owns counts. */
    else result = fvm_swf_stream_sound_snapshot_open_adpcm_counted( /* Decode only explicit lengths. */
        context, 0, block_frames, sizeof(block_frames) / sizeof(block_frames[0]), /* Root block order. */
        EXAMPLE_ADPCM_BUDGET, &source); /* Bound encoded+decoded peak; source cleared on failure. */
    if (!checked(result, "counted preparation")) goto cleanup; /* Never publish partial PCM. */
    fvm_context_destroy(context); context = 0; /* Prove that source no longer borrows a context. */
    fvm_runtime_destroy(runtime); runtime = 0; /* No VM/GC owner remains in the export loop. */
    if (!ranges && !checked(fvm_swf_stream_pcm_reader_init(&reader, source), "reader")) goto cleanup; /* Borrow. */
    file = fopen(argv[1], "wb"); /* Explicitly replace only the requested output file. */
    if (!file) goto cleanup; /* A native file-open failure is terminal for this export. */
    if (ranges) /* Decode selected ranges from retained encoded bytes without new storage. */
    { /* Begin explicit block-ordered native extraction. */
        size_t block; /* Preserve empty block ordinals in the authoritative count table. */
        for (block = 0; block < sizeof(block_frames) / sizeof(block_frames[0]); ++block) /* Each block. */
        { /* Begin one independently encoded block. */
            size_t first = 0; /* Each block restarts predictor/index state. */
            do /* Validate even an empty block once before moving to its successor. */
            { /* Decode a bounded caller-owned transfer range. */
                size_t frames = block_frames[block] - first; /* Remaining authoritative frames. */
                if (frames > EXAMPLE_ADPCM_CHUNK) frames = EXAMPLE_ADPCM_CHUNK; /* Bound output. */
                result = fvm_swf_stream_sound_snapshot_read_adpcm( /* No VM or allocation. */
                    source, block, block_frames[block], first, frames, pcm, /* Full count and range. */
                    sizeof(pcm) / sizeof(pcm[0]), &written); /* Capacity counts scalar values. */
                if (!checked(result, "native ADPCM range")) goto cleanup; /* Terminal source error. */
                if (fwrite(pcm, sizeof(pcm[0]), written, file) != written) goto cleanup; /* Full copy. */
                first += frames; total += frames; /* Advance after complete accepted output. */
            } while (first < block_frames[block]); /* Empty blocks emit no artificial silence. */
        } /* End native block traversal. */
    } /* End encoded-source range mode. */
    else for (;;) /* Copy bounded chunks; empty blocks preserve indices but contribute no samples. */
    { /* Begin native-only streaming, with no callbacks, device, thread or allocation. */
        chunk.struct_size = sizeof(chunk); /* Request the exact supported metadata layout. */
        result = fvm_swf_stream_pcm_reader_next(&reader, EXAMPLE_ADPCM_CHUNK, pcm, /* Copy samples. */
            sizeof(pcm) / sizeof(pcm[0]), &chunk, &written); /* Capacity counts scalar S16 values. */
        if (result == FVM_NOT_FOUND) break; /* Only actual native EOF ends a successful export. */
        if (!checked(result, "read")) goto cleanup; /* Preserve ownership after decode/read errors. */
        if (fwrite(pcm, sizeof(pcm[0]), written, file) != written) goto cleanup; /* No fake retry. */
        total += chunk.frames; /* Advance only after the whole file chunk was accepted. */
    } /* End the finite native reader loop. */
    if (fclose(file) != 0) { file = 0; goto cleanup; } /* Flush errors remain export failures. */
    file = 0; /* Acknowledge file cleanup exactly once. */
    printf("Decoded %lu counted ADPCM frames after VM teardown.\n", (unsigned long)total); /* Evidence. */
    status = 0; /* Every read/write/close completed successfully. */
cleanup: /* Release resources in reverse order; no callback retains the local reader. */
    if (file) fclose(file); /* Best-effort close after a terminal native output failure. */
    fvm_swf_stream_sound_snapshot_close(source); /* Null-safe native release after reader quiescence. */
    if (context) fvm_context_destroy(context); /* Release source on partial preparation failure. */
    if (runtime) fvm_runtime_destroy(runtime); /* End remaining managed ownership last. */
    return status; /* Preserve the complete export result for shell scripts. */
} /* End the counted partial-ADPCM tutorial. */
