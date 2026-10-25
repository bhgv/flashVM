/* Tutorial: prepare counted partial ADPCM, then export native S16 after destroying the VM. */
#include "flashvm/swf_stream_pcm.h" /* Declare counted preparation and independent native reads. */
#include <string.h>                 /* Validate the explicit allocation-free range export option. */
#include <stdio.h>                  /* Open only the explicitly requested native-S16 output file. */
#include "example_movie.h"          /* Own the SWF bytes, authoritative counts and bounded geometry. */


/* Translate result codes into ordinary tutorial error handling; no assertion aborts. */
static int checked(FVM_Result result, const char *operation) /* Keep the failed API name. */
{                                                            /* Begin recoverable status translation. */
    if (result == FVM_OK)                                    /* Condition for this action: Continue after a complete acknowledged operation. */
    {                                                        /* Begin conditional branch: Condition for this action: Continue after a complete acknowledged operation. */
        return 1;                                            /* Continue after a complete acknowledged operation. */
    } /* End conditional branch: Condition for this action: Continue after a complete acknowledged operation. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Preserve the exact error. */
    return 0;                                                   /* Ask the caller to release partial ownership. */
} /* Finish status translation. */


/* Retain a native snapshot until every reader and file write has finished. */
int main(int argc, char **argv)                                                                      /* One optional output path; no hidden input or timing policy. */
{                                                                                                    /* Begin the owner-side tutorial. */
    FVM_Config config;                                                                               /* Runtime configuration is copied during creation. */
    FVM_Runtime *runtime = 0;                                                                        /* Owner releases this after preparing native PCM. */
    FVM_Context *context = 0;                                                                        /* Source inspection stays on this context's owner thread. */
    FVM_SWFStreamSoundSnapshot *source = 0;                                                          /* Independent native snapshot survives VM teardown. */
    FVM_SWFStreamPCMReader reader;                                                                   /* Borrow source until the copy loop has stopped. */
    FVM_SWFStreamPCMChunk chunk;                                                                     /* Receive exact frame metadata for each copied range. */
    FILE *file = 0;                                                                                  /* Caller-requested file, closed on success and every error path. */
    short pcm[EXAMPLE_ADPCM_CHUNK];                                                                  /* Caller-owned aligned mono S16 transfer buffer. */
    size_t written = 0, total = 0;                                                                   /* Count scalar values and accepted mono source frames. */
    FVM_Result result = FVM_OK;                                                                      /* Retain the exact read status at EOF or failure. */
    int inferred = 0;                                                                                /* Choose unique byte-geometry counts instead of the fixture count table. */
    int ranges = 0;                                                                                  /* Choose encoded native ranges instead of eager normalized PCM. */
    int status = 1;                                                                                  /* Default to failure until export and close both succeed. */
    if (argc == 1)                                                                                   /* Usage has no file or VM side effects. */
    {                                                                                                /* Explain the one explicit export action. */
        puts("Usage: swf_stream_adpcm OUTPUT.pcm [--ranges|--inferred] (native S16 mono 44100 Hz)"); /* No output file. */
        return 0;                                                                                    /* A usage request succeeds. */
    } /* End the no-side-effect usage path. */
    if (argc == 3 && strcmp(argv[2], "--ranges") == 0) /* Condition for this action: Explicit raw-read policy. */
    {                                                  /* Begin conditional branch: Condition for this action: Explicit raw-read policy. */
        ranges = 1;                                    /* Explicit raw-read policy. */
    } /* End conditional branch: Condition for this action: Explicit raw-read policy. */
    else if (argc == 3 && strcmp(argv[2], "--inferred") == 0) /* Never use the advisory average. */
    {                                                         /* Begin conditional branch: Never use the advisory average. */
        inferred = 1;                                         /* Never use the advisory average. */
    } /* End conditional branch: Never use the advisory average. */
    else if (argc != 2) /* Reject malformed command lines before allocating or opening files. */
    {                   /* Begin conditional branch: Reject malformed command lines before allocating or opening files. */
        return 1;       /* Reject malformed command lines before allocating or opening files. */
    } /* End conditional branch: Reject malformed command lines before allocating or opening files. */
    if (!checked(fvm_config_init(&config), "config")) /* Condition for this action: Initialize owner policy. */
    {                                                 /* Begin conditional branch: Condition for this action: Initialize owner policy. */
        goto cleanup;                                 /* Initialize owner policy. */
    } /* End conditional branch: Condition for this action: Initialize owner policy. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime")) /* Condition for this action: Acquire VM. */
    {                                                               /* Begin conditional branch: Condition for this action: Acquire VM. */
        goto cleanup;                                               /* Acquire VM. */
    } /* End conditional branch: Condition for this action: Acquire VM. */
    if (!checked(fvm_context_create(runtime, &context), "context")) /* Condition for this action: Acquire source. */
    {                                                               /* Begin conditional branch: Condition for this action: Acquire source. */
        goto cleanup;                                               /* Acquire source. */
    } /* End conditional branch: Condition for this action: Acquire source. */
    if (!checked(fvm_load_memory(context, movie, sizeof(movie)), "load")) /* Condition for this action: Real SWF8. */
    {                                                                     /* Begin conditional branch: Condition for this action: Real SWF8. */
        goto cleanup;                                                     /* Real SWF8. */
    } /* End conditional branch: Condition for this action: Real SWF8. */
    if (ranges)                                                                                         /* Condition for this action: Keep compressed ownership. */
    {                                                                                                   /* Begin conditional branch: Condition for this action: Keep compressed ownership. */
        result = fvm_swf_stream_sound_snapshot_open_bounded(                                            /* Keep compressed ownership. */
                                                            context, 0, EXAMPLE_ADPCM_BUDGET, &source); /* Copy without decoding; caller owns counts. */
    } /* End conditional branch: Condition for this action: Keep compressed ownership. */
    else if (inferred)                                                                                       /* Infer only a unique partial endpoint. */
    {                                                                                                        /* Begin conditional branch: Infer only a unique partial endpoint. */
        result = fvm_swf_stream_sound_snapshot_open_pcm_inferred(                                            /* Infer only a unique partial endpoint. */
                                                                 context, 0, EXAMPLE_ADPCM_BUDGET, &source); /* Preserve the same bounded independent ownership. */
    } /* End conditional branch: Infer only a unique partial endpoint. */
    else                                                                                                                                    /* Decode only explicit lengths. */
    {                                                                                                                                       /* Begin conditional branch: Decode only explicit lengths. */
        result = fvm_swf_stream_sound_snapshot_open_adpcm_counted(                                                                          /* Decode only explicit lengths. */
                                                                  context, 0, block_frames, sizeof(block_frames) / sizeof(block_frames[0]), /* Root block order. */
                                                                  EXAMPLE_ADPCM_BUDGET, &source);                                           /* Bound encoded+decoded peak; source cleared on failure. */
    } /* End conditional branch: Decode only explicit lengths. */
    if (!checked(result, "counted preparation")) /* Condition for this action: Never publish partial PCM. */
    {                                            /* Begin conditional branch: Condition for this action: Never publish partial PCM. */
        goto cleanup;                            /* Never publish partial PCM. */
    } /* End conditional branch: Condition for this action: Never publish partial PCM. */
    fvm_context_destroy(context);                                                       /* Prove that source no longer borrows a context. */
    context = 0;                                                                        /* Prove that source no longer borrows a context. */
    fvm_runtime_destroy(runtime);                                                       /* No VM/GC owner remains in the export loop. */
    runtime = 0;                                                                        /* No VM/GC owner remains in the export loop. */
    if (!ranges && !checked(fvm_swf_stream_pcm_reader_init(&reader, source), "reader")) /* Condition for this action: Borrow. */
    {                                                                                   /* Begin conditional branch: Condition for this action: Borrow. */
        goto cleanup;                                                                   /* Borrow. */
    } /* End conditional branch: Condition for this action: Borrow. */
    file = fopen(argv[1], "wb"); /* Explicitly replace only the requested output file. */
    if (!file)                   /* Condition for this action: A native file-open failure is terminal for this export. */
    {                            /* Begin conditional branch: Condition for this action: A native file-open failure is terminal for this export. */
        goto cleanup;            /* A native file-open failure is terminal for this export. */
    } /* End conditional branch: Condition for this action: A native file-open failure is terminal for this export. */
    if (ranges)                                                                          /* Decode selected ranges from retained encoded bytes without new storage. */
    {                                                                                    /* Begin explicit block-ordered native extraction. */
        size_t block;                                                                    /* Preserve empty block ordinals in the authoritative count table. */
        for (block = 0; block < sizeof(block_frames) / sizeof(block_frames[0]); ++block) /* Each block. */
        {                                                                                /* Begin one independently encoded block. */
            size_t first = 0;                                                            /* Each block restarts predictor/index state. */
            do                                                                           /* Validate even an empty block once before moving to its successor. */
            {                                                                            /* Decode a bounded caller-owned transfer range. */
                size_t frames = block_frames[block] - first;                             /* Remaining authoritative frames. */
                if (frames > EXAMPLE_ADPCM_CHUNK)                                        /* Condition for this action: Bound output. */
                {                                                                        /* Begin conditional branch: Condition for this action: Bound output. */
                    frames = EXAMPLE_ADPCM_CHUNK;                                        /* Bound output. */
                } /* End conditional branch: Condition for this action: Bound output. */
                result = fvm_swf_stream_sound_snapshot_read_adpcm(                                                        /* No VM or allocation. */
                                                                  source, block, block_frames[block], first, frames, pcm, /* Full count and range. */
                                                                  sizeof(pcm) / sizeof(pcm[0]), &written);                /* Capacity counts scalar values. */
                if (!checked(result, "native ADPCM range"))                                                               /* Condition for this action: Terminal source error. */
                {                                                                                                         /* Begin conditional branch: Condition for this action: Terminal source error. */
                    goto cleanup;                                                                                         /* Terminal source error. */
                } /* End conditional branch: Condition for this action: Terminal source error. */
                if (fwrite(pcm, sizeof(pcm[0]), written, file) != written) /* Condition for this action: Full copy. */
                {                                                          /* Begin conditional branch: Condition for this action: Full copy. */
                    goto cleanup;                                          /* Full copy. */
                } /* End conditional branch: Condition for this action: Full copy. */
                first += frames; /* Advance after complete accepted output. */
                total += frames; /* Advance after complete accepted output. */
            } while (first < block_frames[block]); /* Empty blocks emit no artificial silence. */
        } /* End native block traversal. */
    } /* End encoded-source range mode. */
    else                                                                                             /* Copy bounded chunks; empty blocks preserve indices but contribute no samples. */
    {                                                                                                /* Begin conditional branch: Copy bounded chunks; empty blocks preserve indices but contribute no samples. */
        for (;;)                                                                                     /* Copy bounded chunks; empty blocks preserve indices but contribute no samples. */
        {                                                                                            /* Begin native-only streaming, with no callbacks, device, thread or allocation. */
            chunk.struct_size = sizeof(chunk);                                                       /* Request the exact supported metadata layout. */
            result = fvm_swf_stream_pcm_reader_next(&reader, EXAMPLE_ADPCM_CHUNK, pcm,               /* Copy samples. */
                                                    sizeof(pcm) / sizeof(pcm[0]), &chunk, &written); /* Capacity counts scalar S16 values. */
            if (result == FVM_NOT_FOUND)                                                             /* Condition for this action: Only actual native EOF ends a successful export. */
            {                                                                                        /* Begin conditional branch: Condition for this action: Only actual native EOF ends a successful export. */
                break;                                                                               /* Only actual native EOF ends a successful export. */
            } /* End conditional branch: Condition for this action: Only actual native EOF ends a successful export. */
            if (!checked(result, "read")) /* Condition for this action: Preserve ownership after decode/read errors. */
            {                             /* Begin conditional branch: Condition for this action: Preserve ownership after decode/read errors. */
                goto cleanup;             /* Preserve ownership after decode/read errors. */
            } /* End conditional branch: Condition for this action: Preserve ownership after decode/read errors. */
            if (fwrite(pcm, sizeof(pcm[0]), written, file) != written) /* Condition for this action: No fake retry. */
            {                                                          /* Begin conditional branch: Condition for this action: No fake retry. */
                goto cleanup;                                          /* No fake retry. */
            } /* End conditional branch: Condition for this action: No fake retry. */
            total += chunk.frames; /* Advance only after the whole file chunk was accepted. */
        } /* End the finite native reader loop. */
    } /* End conditional branch: Copy bounded chunks; empty blocks preserve indices but contribute no samples. */
    if (fclose(file) != 0) /* Condition for this action: Flush errors remain export failures. */
    {                      /* Begin conditional branch: Condition for this action: Flush errors remain export failures. */
        file = 0;          /* Flush errors remain export failures. */
        goto cleanup;      /* Flush errors remain export failures. */
    } /* Flush errors remain export failures. */
    file = 0;                                                                                                            /* Acknowledge file cleanup exactly once. */
    printf("Decoded %lu %s ADPCM frames after VM teardown.\n", (unsigned long)total, inferred ? "inferred" : "counted"); /* Evidence. */
    status = 0;                                                                                                          /* Every read/write/close completed successfully. */
cleanup:                                                                                                                 /* Release resources in reverse order; no callback retains the local reader. */
    if (file)                                                                                                            /* Condition for this action: Best-effort close after a terminal native output failure. */
    {                                                                                                                    /* Begin conditional branch: Condition for this action: Best-effort close after a terminal native output failure. */
        fclose(file);                                                                                                    /* Best-effort close after a terminal native output failure. */
    } /* End conditional branch: Condition for this action: Best-effort close after a terminal native output failure. */
    fvm_swf_stream_sound_snapshot_close(source); /* Null-safe native release after reader quiescence. */
    if (context)                                 /* Condition for this action: Release source on partial preparation failure. */
    {                                            /* Begin conditional branch: Condition for this action: Release source on partial preparation failure. */
        fvm_context_destroy(context);            /* Release source on partial preparation failure. */
    } /* End conditional branch: Condition for this action: Release source on partial preparation failure. */
    if (runtime)                      /* Condition for this action: End remaining managed ownership last. */
    {                                 /* Begin conditional branch: Condition for this action: End remaining managed ownership last. */
        fvm_runtime_destroy(runtime); /* End remaining managed ownership last. */
    } /* End conditional branch: Condition for this action: End remaining managed ownership last. */
    return status; /* Preserve the complete export result for shell scripts. */
} /* End the counted partial-ADPCM tutorial. */
