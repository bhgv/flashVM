/* C89 tutorial: decode once, destroy the VM, clone/restart and export two finite native replays. */
#include "flashvm/swf_sound_mixer.h" /* Independent voice, budget and mixer ownership APIs. */
#include "example_constants.h"       /* Explicit input, storage and finite-output policy. */
#include <stdio.h>                   /* Open only caller-selected binary files. */
#include <stdlib.h>                  /* Own the bounded temporary input copy. */


/* Keep actual errors visible while preserving one centralized cleanup path. */
static int checked(FVM_Result result, const char *operation) /* Translate one API result. */
{                                                            /* Begin status reporting. */
    if (result == FVM_OK)                                    /* Condition for this action: Continue only after acknowledged success. */
    {                                                        /* Begin conditional branch: Condition for this action: Continue only after acknowledged success. */
        return 1;                                            /* Continue only after acknowledged success. */
    } /* End conditional branch: Condition for this action: Continue only after acknowledged success. */
    fprintf(stderr, "%s: %d\n", operation, (int)result); /* Preserve the original result. */
    return 0;                                            /* Let main release its partial owners. */
} /* End status reporting. */


/* All source VM work finishes before native clones, mixer commands or output file acquisition. */
int main(int argc, char **argv)                                      /* INPUT.swf OUTPUT.pcm selects the first root StartSound. */
{                                                                    /* Begin caller-owned replay lifetime. */
    FVM_Config config;                                               /* Initialized bounded source runtime policy. */
    FVM_Runtime *runtime = 0;                                        /* Destroy after its context, before native replay. */
    FVM_Context *context = 0;                                        /* Needed only to prepare the template. */
    FVM_SWFSoundVoice *source = 0;                                   /* Immutable template retained independently of the VM. */
    FVM_SWFSoundVoice *copy = 0;                                     /* Untransferred replay owner; null after successful adoption. */
    FVM_SWFSoundMixer *mixer = 0;                                    /* Owns only the current adopted replay. */
    FVM_SWFSoundVoiceInfo info;                                      /* Explicit finite duration and effective source geometry. */
    FVM_SWFSoundVoiceStorageInfo storage;                            /* Exact retained template bytes. */
    FVM_SWFSoundMixerStorageInfo mixer_storage;                      /* Fixed mixer charge before every clone. */
    FVM_SWFSoundMixerConfig geometry;                                /* Same-clock native output, no source-rate conversion. */
    FILE *input = 0, *output = 0;                                    /* Separate explicitly named file owners. */
    unsigned char *bytes = 0;                                        /* Temporary input storage released after load. */
    long length = 0;                                                 /* Validate the signed ftell result before casting. */
    unsigned long duration = 0, total = 0, position = 0;             /* Checked finite frame counts. */
    size_t available = 0, clone_budget = 0, frames = 0, written = 0; /* Requested native storage and scalar output. */
    short pcm[EXAMPLE_REUSE_BLOCK * EXAMPLE_REUSE_CHANNELS];         /* Disjoint caller transfer storage. */
    unsigned int pass = 0;                                           /* Count independent replay owners, never duplicate decode calls. */
    int status = 1;                                                  /* Fail unless every read/write/close completes. */
    if (argc == 1)                                                   /* Usage has no VM or filesystem side effects. */
    {                                                                /* Explain the explicit output destination. */
        puts("Usage: swf_voice_reuse INPUT.swf OUTPUT.pcm");         /* Native-endian interleaved S16 output. */
        return 0;                                                    /* A usage request succeeds. */
    } /* End usage-only mode. */
    if (argc != 3) /* Condition for this action: Reject malformed CLI before opening a file. */
    {              /* Begin conditional branch: Condition for this action: Reject malformed CLI before opening a file. */
        return 1;  /* Reject malformed CLI before opening a file. */
    } /* End conditional branch: Condition for this action: Reject malformed CLI before opening a file. */
    input = fopen(argv[1], "rb");                 /* Acquire only the requested source. */
    if (!input || fseek(input, 0, SEEK_END) != 0) /* Condition for this action: Require a seekable regular input. */
    {                                             /* Begin conditional branch: Condition for this action: Require a seekable regular input. */
        goto cleanup;                             /* Require a seekable regular input. */
    } /* End conditional branch: Condition for this action: Require a seekable regular input. */
    length = ftell(input);                                                /* Determine the complete source size. */
    if (length <= 0 || (unsigned long)length > EXAMPLE_REUSE_INPUT_BYTES) /* Condition for this action: Bound source. */
    {                                                                     /* Begin conditional branch: Condition for this action: Bound source. */
        goto cleanup;                                                     /* Bound source. */
    } /* End conditional branch: Condition for this action: Bound source. */
    if (fseek(input, 0, SEEK_SET) != 0) /* Condition for this action: Restore the exact input start. */
    {                                   /* Begin conditional branch: Condition for this action: Restore the exact input start. */
        goto cleanup;                   /* Restore the exact input start. */
    } /* End conditional branch: Condition for this action: Restore the exact input start. */
    bytes = (unsigned char *)malloc((size_t)length);                        /* Acquire one bounded input copy. */
    if (!bytes || fread(bytes, 1, (size_t)length, input) != (size_t)length) /* Condition for this action: No partial load. */
    {                                                                       /* Begin conditional branch: Condition for this action: No partial load. */
        goto cleanup;                                                       /* No partial load. */
    } /* End conditional branch: Condition for this action: No partial load. */
    if (fclose(input) != 0) /* Condition for this action: Preserve close failures. */
    {                       /* Begin conditional branch: Condition for this action: Preserve close failures. */
        input = 0;          /* Preserve close failures. */
        goto cleanup;       /* Preserve close failures. */
    } /* Preserve close failures. */
    input = 0;                                        /* End file ownership once. */
    if (!checked(fvm_config_init(&config), "config")) /* Condition for this action: Initialize all fields. */
    {                                                 /* Begin conditional branch: Condition for this action: Initialize all fields. */
        goto cleanup;                                 /* Initialize all fields. */
    } /* End conditional branch: Condition for this action: Initialize all fields. */
    config.memory_limit_bytes = EXAMPLE_REUSE_NATIVE_BYTES;         /* Bound source runtime payloads. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime")) /* Condition for this action: Own runtime. */
    {                                                               /* Begin conditional branch: Condition for this action: Own runtime. */
        goto cleanup;                                               /* Own runtime. */
    } /* End conditional branch: Condition for this action: Own runtime. */
    if (!checked(fvm_context_create(runtime, &context), "context")) /* Condition for this action: Own source context. */
    {                                                               /* Begin conditional branch: Condition for this action: Own source context. */
        goto cleanup;                                               /* Own source context. */
    } /* End conditional branch: Condition for this action: Own source context. */
    if (!checked(fvm_load_memory(context, bytes, (size_t)length), "load")) /* Condition for this action: Copy source. */
    {                                                                      /* Begin conditional branch: Condition for this action: Copy source. */
        goto cleanup;                                                      /* Copy source. */
    } /* End conditional branch: Condition for this action: Copy source. */
    free(bytes);                                                                        /* No caller file bytes survive successful preparation. */
    bytes = 0;                                                                          /* No caller file bytes survive successful preparation. */
    if (!checked(fvm_swf_sound_voice_open_bounded(context, 0,                           /* Decode exactly once on the owner. */
                                                  EXAMPLE_REUSE_NATIVE_BYTES, &source), /* Retain native interval/style. */
                 "template"))                                                           /* Retain native interval/style. */
    {                                                                                   /* Begin scope: Retain native interval/style. */
        goto cleanup;                                                                   /* Retain native interval/style. */
    } /* End scope: Retain native interval/style. */
    info.struct_size = sizeof(info);                                   /* Request exact copied playback metadata. */
    if (!checked(fvm_swf_sound_voice_info(source, &info), "duration")) /* Condition for this action: Effective clock. */
    {                                                                  /* Begin conditional branch: Condition for this action: Effective clock. */
        goto cleanup;                                                  /* Effective clock. */
    } /* End conditional branch: Condition for this action: Effective clock. */
    if (info.play_count && info.frames_per_play >                                                    /* Bound multiplication before computing duration. */
                               EXAMPLE_REUSE_EXPORT_FRAMES / EXAMPLE_REUSE_PASSES / info.play_count) /* Reject huge loops. */
    {                                                                                                /* Begin scope: Reject huge loops. */
        goto cleanup;                                                                                /* Reject huge loops. */
    } /* End scope: Reject huge loops. */
    duration = (unsigned long)info.frames_per_play * info.play_count;                   /* One finite complete replay. */
    fvm_context_destroy(context);                                                       /* No native owner borrows managed objects. */
    context = 0;                                                                        /* No native owner borrows managed objects. */
    fvm_runtime_destroy(runtime);                                                       /* Complete VM teardown before cloning. */
    runtime = 0;                                                                        /* Complete VM teardown before cloning. */
    storage.struct_size = sizeof(storage);                                              /* Request native object/array charges. */
    if (!checked(fvm_swf_sound_voice_storage_info(source, &storage), "template bytes")) /* Condition for this action: No VM. */
    {                                                                                   /* Begin conditional branch: Condition for this action: No VM. */
        goto cleanup;                                                                   /* No VM. */
    } /* End conditional branch: Condition for this action: No VM. */
    if (storage.retained_bytes > EXAMPLE_REUSE_NATIVE_BYTES) /* Condition for this action: Guard subtraction. */
    {                                                        /* Begin conditional branch: Condition for this action: Guard subtraction. */
        goto cleanup;                                        /* Guard subtraction. */
    } /* End conditional branch: Condition for this action: Guard subtraction. */
    available = EXAMPLE_REUSE_NATIVE_BYTES - storage.retained_bytes;                         /* Reserve template ownership first. */
    geometry.struct_size = sizeof(geometry);                                                 /* Initialize exact configured mixer ABI. */
    geometry.sample_rate = info.sample_rate;                                                 /* Keep source geometry. */
    geometry.channels = info.channels;                                                       /* Keep source geometry. */
    geometry.voice_capacity = EXAMPLE_REUSE_VOICES;                                          /* Bound active replay count. */
    geometry.flags = FVM_SWF_SOUND_MIXER_CONVERT;                                            /* Permit effective MPEG clocks without nominal-rate rejection. */
    if (!checked(fvm_swf_sound_mixer_create_bounded(&geometry, available, &mixer), "mixer")) /* Condition for this action: Own native mixer. */
    {                                                                                        /* Begin conditional branch: Condition for this action: Own native mixer. */
        goto cleanup;                                                                        /* Own native mixer. */
    } /* End conditional branch: Condition for this action: Own native mixer. */
    mixer_storage.struct_size = sizeof(mixer_storage);                                    /* Query fixed object/slot accounting. */
    if (!checked(fvm_swf_sound_mixer_storage_info(mixer, &mixer_storage), "mixer bytes")) /* Condition for this action: No device. */
    {                                                                                     /* Begin conditional branch: Condition for this action: No device. */
        goto cleanup;                                                                     /* No device. */
    } /* End conditional branch: Condition for this action: No device. */
    if (mixer_storage.retained_bytes > available) /* Condition for this action: Guard the remaining clone allowance. */
    {                                             /* Begin conditional branch: Condition for this action: Guard the remaining clone allowance. */
        goto cleanup;                             /* Guard the remaining clone allowance. */
    } /* End conditional branch: Condition for this action: Guard the remaining clone allowance. */
    clone_budget = available - mixer_storage.retained_bytes;                                        /* Include all simultaneously retained owners. */
    if (duration && !checked(fvm_swf_sound_voice_seek(source, info.play_count, 0), "template EOF")) /* Condition for this action: Keep completed template. */
    {                                                                                               /* Begin conditional branch: Condition for this action: Keep completed template. */
        goto cleanup;                                                                               /* Keep completed template. */
    } /* End conditional branch: Condition for this action: Keep completed template. */
    for (pass = 0; pass < EXAMPLE_REUSE_PASSES; ++pass)                                /* Each replay owns a fresh independent native cursor. */
    {                                                                                  /* Begin VM-free deep copy and transfer. */
        if (!checked(fvm_swf_sound_voice_clone(source, clone_budget, &copy), "clone")) /* Condition for this action: No decoder. */
        {                                                                              /* Begin conditional branch: Condition for this action: No decoder. */
            goto cleanup;                                                              /* No decoder. */
        } /* End conditional branch: Condition for this action: No decoder. */
        if (!checked(fvm_swf_sound_voice_seek(copy, 0, 0), "restart clone")) /* Condition for this action: Restore complete authored playback. */
        {                                                                    /* Begin conditional branch: Condition for this action: Restore complete authored playback. */
            goto cleanup;                                                    /* Restore complete authored playback. */
        } /* End conditional branch: Condition for this action: Restore complete authored playback. */
        if (!checked(fvm_swf_sound_mixer_submit_prepared_bounded(mixer, &copy, available), "adopt")) /* Condition for this action: Consume only on OK. */
        {                                                                                            /* Begin conditional branch: Condition for this action: Consume only on OK. */
            goto cleanup;                                                                            /* Consume only on OK. */
        } /* End conditional branch: Condition for this action: Consume only on OK. */
        if (!output)                       /* Condition for this action: Replace requested output only after first successful adoption. */
        {                                  /* Begin conditional branch: Condition for this action: Replace requested output only after first successful adoption. */
            output = fopen(argv[2], "wb"); /* Replace requested output only after first successful adoption. */
        } /* End conditional branch: Condition for this action: Replace requested output only after first successful adoption. */
        if (!output)      /* Condition for this action: Terminal native file failure. */
        {                 /* Begin conditional branch: Condition for this action: Terminal native file failure. */
            goto cleanup; /* Terminal native file failure. */
        } /* End conditional branch: Condition for this action: Terminal native file failure. */
        for (position = 0; position < duration; position += (unsigned long)frames) /* No infinite mixer silence. */
        {                                                                          /* Begin finite caller-authorized PCM copy. */
            frames = (size_t)(duration - position);                                /* Remaining frames in this replay. */
            if (frames > EXAMPLE_REUSE_BLOCK)                                      /* Condition for this action: Bound native work. */
            {                                                                      /* Begin conditional branch: Condition for this action: Bound native work. */
                frames = EXAMPLE_REUSE_BLOCK;                                      /* Bound native work. */
            } /* End conditional branch: Condition for this action: Bound native work. */
            if (!checked(fvm_swf_sound_mixer_read(mixer, frames, pcm,                      /* Advance only independent native data. */
                                                  sizeof(pcm) / sizeof(pcm[0]), &written), /* Scalar capacity/output. */
                         "read"))                                                          /* Scalar capacity/output. */
            {                                                                              /* Begin scope: Scalar capacity/output. */
                goto cleanup;                                                              /* Scalar capacity/output. */
            } /* End scope: Scalar capacity/output. */
            if (fwrite(pcm, sizeof(pcm[0]), written, output) != written) /* Condition for this action: Short write is terminal. */
            {                                                            /* Begin conditional branch: Condition for this action: Short write is terminal. */
                goto cleanup;                                            /* Short write is terminal. */
            } /* End conditional branch: Condition for this action: Short write is terminal. */
            total += (unsigned long)frames; /* Count only completely accepted file PCM. */
        } /* End this finite replay. */
    } /* End both independent native owners. */
    if (fclose(output) != 0) /* Condition for this action: Observe final flush errors. */
    {                        /* Begin conditional branch: Condition for this action: Observe final flush errors. */
        output = 0;          /* Observe final flush errors. */
        goto cleanup;        /* Observe final flush errors. */
    } /* Observe final flush errors. */
    output = 0;                                                                 /* End output ownership once. */
    printf("Exported %lu frames at %u Hz/%u channels in %u VM-free replays.\n", /* Native sample geometry. */
           total, info.sample_rate, info.channels, EXAMPLE_REUSE_PASSES);       /* Explain exact expected output. */
    status = 0;                                                                 /* Every explicit operation completed. */
cleanup:                                                                        /* Quiesce all readers before releasing borrowed owners. */
    if (input)                                                                  /* Condition for this action: Release partial input acquisition. */
    {                                                                           /* Begin conditional branch: Condition for this action: Release partial input acquisition. */
        fclose(input);                                                          /* Release partial input acquisition. */
    } /* End conditional branch: Condition for this action: Release partial input acquisition. */
    if (output)         /* Condition for this action: Failure may leave partial output, explicitly documented. */
    {                   /* Begin conditional branch: Condition for this action: Failure may leave partial output, explicitly documented. */
        fclose(output); /* Failure may leave partial output, explicitly documented. */
    } /* End conditional branch: Condition for this action: Failure may leave partial output, explicitly documented. */
    free(bytes);                       /* Release any unconsumed temporary source copy. */
    fvm_swf_sound_voice_close(copy);   /* Release rejected/untransferred clone only. */
    fvm_swf_sound_mixer_close(mixer);  /* Release adopted clone after all reads stop. */
    fvm_swf_sound_voice_close(source); /* End template ownership after the last clone finishes. */
    if (context)                       /* Condition for this action: Release partial managed preparation. */
    {                                  /* Begin conditional branch: Condition for this action: Release partial managed preparation. */
        fvm_context_destroy(context);  /* Release partial managed preparation. */
    } /* End conditional branch: Condition for this action: Release partial managed preparation. */
    if (runtime)                      /* Condition for this action: Release source runtime last if still present. */
    {                                 /* Begin conditional branch: Condition for this action: Release source runtime last if still present. */
        fvm_runtime_destroy(runtime); /* Release source runtime last if still present. */
    } /* End conditional branch: Condition for this action: Release source runtime last if still present. */
    return status; /* Report every terminal preparation/output failure. */
} /* End deterministic native replay tutorial. */
