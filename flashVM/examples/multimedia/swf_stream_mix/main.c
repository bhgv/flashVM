/* Tutorial: mix real SWF stream PCM and StartSound snapshots after source VM teardown. */
#include "flashvm/swf_stream_output.h"   /* Coordinate source frames and event-only gates with one FIFO. */
#include "flashvm/swf_stream_convert.h"  /* Read native stream blocks on an explicit output clock. */
#include "flashvm/swf_sound_mix_queue.h" /* Retain mixed bytes through explicit queue rejection. */
#include "flashvm/swf_sound_mixer.h"     /* Add event voices before a single saturation pass. */
#include "example_constants.h"           /* Keep tutorial geometry distinct from wire constants. */
#include <stdio.h>                       /* Write only the explicitly selected PCM output file. */
#include <string.h>                      /* Initialize zero base for an explicit event-only tail. */


/* Report recoverable API failures without tutorial assertions. */
static int checked(FVM_Result result, const char *operation) /* All error paths retain cleanup. */
{                                                            /* Begin result translation. */
    if (result == FVM_OK)                                    /* Condition for this action: Continue only after acknowledged success. */
    {                                                        /* Begin conditional branch: Condition for this action: Continue only after acknowledged success. */
        return 1;                                            /* Continue only after acknowledged success. */
    } /* End conditional branch: Condition for this action: Continue only after acknowledged success. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Retain the exact result code. */
    return 0;                                                   /* Request centralized cleanup. */
} /* Finish result translation. */

/* A bounded real native FIFO accepts all frames or none; file consumption happens later. */
typedef struct ExampleQueue                                     /* Keep all callback userdata alive until adapter close. */
{                                                               /* Begin explicit queue ownership. */
    short pcm[EXAMPLE_MIX_QUEUE_FRAMES * EXAMPLE_MIX_CHANNELS]; /* Own the finite accepted-frame store. */
    size_t frames;                                              /* Current initialized complete-frame count. */
    size_t rejected;                                            /* Count deliberate transient rejections before any sample copy. */
    int reject;                                                 /* Owner arms one controlled rejection for the next input block. */
    int paused;                                                 /* Last acknowledged software pause state; no device is implied. */
} ExampleQueue;                                                 /* Finish fixed-capacity native state. */


/* An accepted write copies everything; a rejected write changes no FIFO bytes or count. */
static FVM_Result queue_write(void *raw, const short *pcm, size_t frames) /* Serial consumer. */
{                                                                         /* Begin atomic software acceptance. */
    ExampleQueue *queue = (ExampleQueue *)raw;                            /* Recover bounded native ownership. */
    if (queue->reject)                                                    /* Reject before touching the actual FIFO. */
    {                                                                     /* One deliberate transient failure must not remix event voices. */
        queue->reject = 0;                                                /* Record one rejection without consuming samples. */
        ++queue->rejected;                                                /* Record one rejection without consuming samples. */
        return FVM_NOT_FOUND;                                             /* No frames were accepted; identical retry is permitted. */
    } /* End deliberate rejection. */
    if (frames > EXAMPLE_MIX_QUEUE_FRAMES - queue->frames) /* Condition for this action: Preserve bounds. */
    {                                                      /* Begin conditional branch: Condition for this action: Preserve bounds. */
        return FVM_OUT_OF_MEMORY;                          /* Preserve bounds. */
    } /* End conditional branch: Condition for this action: Preserve bounds. */


    memcpy(queue->pcm + queue->frames * EXAMPLE_MIX_CHANNELS, pcm, /* Copy accepted source bytes. */
           frames * EXAMPLE_MIX_CHANNELS * sizeof(short));         /* Retain no pointer into adapter storage. */
    queue->frames += frames;                                       /* Publish count only after the complete copy. */
    return FVM_OK;                                                 /* Acknowledge ownership transfer to the software FIFO. */
} /* End atomic software acceptance. */


/* Observe complete native FIFO frames without changing consumption or voice time. */
static FVM_Result queue_count(void *raw, size_t *out) /* The adapter validates output first. */
{                                                     /* Begin native observation. */
    *out = ((ExampleQueue *)raw)->frames;             /* Report the initialized complete-frame prefix. */
    return FVM_OK;                                    /* No device or VM is consulted. */
} /* End native observation. */


/* Keep pause acknowledgement explicit even though this tutorial has no timed consumer. */
static FVM_Result queue_pause(void *raw, int paused) /* Adapter validates zero or one. */
{                                                    /* Begin software control. */
    ((ExampleQueue *)raw)->paused = paused;          /* Store the acknowledged policy. */
    return FVM_OK;                                   /* No implicit FIFO clear occurs. */
} /* End software control. */


/* Clear is atomic because no file consumer has run while the adapter is active. */
static FVM_Result queue_clear(void *raw) /* No pending FILE side effect is rolled back. */
{                                        /* Begin native cancellation. */
    ((ExampleQueue *)raw)->frames = 0;   /* Discard the initialized prefix by acknowledged policy. */
    return FVM_OK;                       /* The composition adapter may now discard its own pending retry. */
} /* End native cancellation. */

/* One exact SWF8 contains two event resources and a root PCM block, all at 44100 Hz. */
static const unsigned char movie[] = /* Keep input reproducible without an encoder. */
    { /* Begin scope: Keep input reproducible without an encoder. */
        /* Begin literal SWF bytes. */
        0x46, 0x57, 0x53, 0x08, 0x6e, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x0c, /* Embedded bytes 0 through 11. */
        0x01, 0x00, 0x97, 0x03, 0x07, 0x00, 0x3e, 0x08, 0x00, 0x00, 0x00, 0x30, /* Embedded bytes 12 through 23. */
        0x75, 0x30, 0x75, 0x30, 0x75, 0x30, 0x75, 0x30, 0x75, 0x30, 0x75, 0x30, /* Embedded bytes 24 through 35. */
        0x75, 0x30, 0x75, 0x9f, 0x03, 0x08, 0x00, 0x3e, 0x0c, 0x00, 0x00, 0x00, /* Embedded bytes 36 through 47. */
        0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, /* Embedded bytes 48 through 59. */
        0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, /* Embedded bytes 60 through 71. */
        0xc3, 0x03, 0x07, 0x00, 0x00, 0xc3, 0x03, 0x08, 0x00, 0x00, 0x44, 0x0b, /* Embedded bytes 72 through 83. */
        0x0e, 0x3e, 0x08, 0x00, 0xd0, 0x04, 0x30, 0x75, 0xd0, 0x8a, 0x20, 0x4e, /* Embedded bytes 84 through 95. */
        0xe0, 0xb1, 0x10, 0x27, 0xf0, 0xd8, 0x30, 0x75, 0xd0, 0x8a, 0x40, 0x00, /* Embedded bytes 96 through 107. */
        0x00, 0x00,                                                             /* Embedded bytes 108 through 109. */
}; /* End the complete canonical input. */


/* Export a finite host-selected mixture; source ownership ends before any file is opened. */
int main(int argc, char **argv)                                                                                                                                                                                                                                             /* Keep the reusable integration valid C89. */
{                                                                                                                                                                                                                                                                           /* Begin VM, independent native sources and file ownership. */
    FVM_Config config;                                                                                                                                                                                                                                                      /* Configure source loading before creating its runtime. */
    FVM_Runtime *runtime = 0;                                                                                                                                                                                                                                               /* Own until all independent snapshots have been prepared. */
    FVM_Context *context = 0;                                                                                                                                                                                                                                               /* Canonical SWF owner, never published to a native worker. */
    FVM_SWFStreamSoundSnapshot *source = 0;                                                                                                                                                                                                                                 /* Own the independently bounded stream copy. */
    FVM_SWFStreamOutput gated_output;                                                                                                                                                                                                                                       /* Coordinate base and events without modifying private producer fields. */
    FVM_SWFStreamConverter converter;                                                                                                                                                                                                                                       /* Borrow source until export completes. */
    FVM_SWFStreamConvertedChunk chunk;                                                                                                                                                                                                                                      /* Receive per-block source/frame metadata. */
    FVM_SWFSoundMixerConfig geometry;                                                                                                                                                                                                                                       /* Select explicit conversion of both event voices. */
    FVM_SWFSoundVoice *prepared[EXAMPLE_MIX_VOICES] = {0};                                                                                                                                                                                                                  /* Optional tracked voices stay caller-owned until adoption succeeds. */
    FVM_SWFStreamMixInfo paired;                                                                                                                                                                                                                                            /* Copy stream and origin counters from one native FIFO observation. */
    FVM_SWFSoundMixQueueOriginInfo origin;                                                                                                                                                                                                                                  /* Copy distinct accepted/dequeued BASE and TAIL categories. */
    FVM_SWFSoundMixQueueIdentityInfo identity;                                                                                                                                                                                                                              /* Copy one sound-ID deadline before and after software drain. */
    FVM_SWFSoundMixer *mixer = 0;                                                                                                                                                                                                                                           /* Own event snapshots and their independent clocks. */
    short pcm[EXAMPLE_MIX_BLOCK * EXAMPLE_MIX_CHANNELS];                                                                                                                                                                                                                    /* Own base, mixed output and retry bytes. */
    FVM_SWFSoundMixQueue *mix_queue = 0;                                                                                                                                                                                                                                    /* Own retry storage before native publication. */
    FVM_AudioQueueBackend backend, adapted;                                                                                                                                                                                                                                 /* Borrow real FIFO callbacks and wrapped composition. */
    FVM_SWFSoundMixQueueInfo pending;                                                                                                                                                                                                                                       /* Observe retained retry ownership without a callback. */
    FVM_SWFSoundMixQueueClock shared_clock;                                                                                                                                                                                                                                 /* Copy shared counters rather than inferring time from stream EOF. */
    FVM_SWFSoundMixQueuePending ownership;                                                                                                                                                                                                                                  /* Observe whether base or tail owns a rendered retry. */
    FVM_SWFSoundMixQueueStorageInfo storage;                                                                                                                                                                                                                                /* Copy fixed native allocation requests for the bounded mode. */
    size_t queue_budget = 0;                                                                                                                                                                                                                                                /* Keep adapter storage separate from stream/voice/device budgets. */
    ExampleQueue queue;                                                                                                                                                                                                                                                     /* Keep the finite native FIFO alive through adapter cleanup. */
    int origins_mode = argc == 3 && !strcmp(argv[2], "--origins");                                                                                                                                                                                                          /* Add bounded BASE/TAIL bookkeeping to the gated source handoff. */
    int finite_mode = argc == 3 && !strcmp(argv[2], "--finite");                                                                                                                                                                                                            /* End event-only output at its live horizon without final block padding. */
    int headroom_mode = argc == 3 && !strcmp(argv[2], "--headroom");                                                                                                                                                                                                        /* Export event-only frames with one-frame FIFO headroom and larger retry caps. */
    int gated_mode = (argc == 3 && !strcmp(argv[2], "--gated")) || origins_mode;                                                                                                                                                                                            /* Hold base while event voices advance, then open its frame gate. */
    int identity_mode = (argc == 3 && !strcmp(argv[2], "--identities")) || gated_mode || headroom_mode || finite_mode;                                                                                                                                                      /* Keep sound IDs active through queued/pending PCM. */
    int clock_mode = (argc == 3 && !strcmp(argv[2], "--clock")) || identity_mode;                                                                                                                                                                                           /* Observe one shared accepted/dequeued epoch across base and tails. */
    int bounded_mode = (argc == 3 && !strcmp(argv[2], "--bounded-tails")) || clock_mode;                                                                                                                                                                                    /* Use an exact adapter-only allocation ceiling. */
    int tail_mode = (argc == 3 && !strcmp(argv[2], "--tails")) || bounded_mode;                                                                                                                                                                                             /* Select explicit pump_tail after actual converter EOF. */
    int queue_mode = (argc == 3 && !strcmp(argv[2], "--queue")) || tail_mode;                                                                                                                                                                                               /* Explicit queue/retry tutorial. */
    FILE *output = 0;                                                                                                                                                                                                                                                       /* No file exists until successful source preparation and teardown. */
    size_t total = 0, count, values, mixed, event, accepted = 0;                                                                                                                                                                                                            /* Distinguish frames from channel values. */
    int eof = 0, status = 1;                                                                                                                                                                                                                                                /* Stop stream reads at EOF; report failure until fclose succeeds. */
    FVM_Result result;                                                                                                                                                                                                                                                      /* Distinguish normal converter EOF from actual failure. */
    memset(&queue, 0, sizeof(queue));                                                                                                                                                                                                                                       /* Begin with no initialized FIFO frames. */
    if (argc != 2 && !queue_mode)                                                                                                                                                                                                                                           /* Usage must not create an output file or start a VM. */
    {                                                                                                                                                                                                                                                                       /* Describe the explicit file replacement. */
        printf("Usage: %s OUTPUT.pcm [--queue|--tails|--bounded-tails|--clock|--identities|--gated|--headroom|--finite|--origins]\nReplaces OUTPUT.pcm with 16 native S16 stereo frames at 48000 Hz (17 with --gated/--origins, 14 with --headroom/--finite).\n", argv[0]); /* State output geometry. */
        return argc == 1 ? 0 : 1;                                                                                                                                                                                                                                           /* Preserve no-side-effect help and reject extra arguments. */
    } /* Finish CLI validation. */
    if (!checked(fvm_config_init(&config), "config")) /* Initialize before any acquisition. */
    {                                                 /* Begin conditional branch: Initialize before any acquisition. */
        goto cleanup;                                 /* No owner exists on this failure path. */
    } /* End conditional branch: Initialize before any acquisition. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime")) /* Acquire source runtime. */
    {                                                               /* Begin conditional branch: Acquire source runtime. */
        goto cleanup;                                               /* Keep source ownership explicit. */
    } /* End conditional branch: Acquire source runtime. */
    if (!checked(fvm_context_create(runtime, &context), "context")) /* Acquire canonical owner. */
    {                                                               /* Begin conditional branch: Acquire canonical owner. */
        goto cleanup;                                               /* Release runtime after a context failure. */
    } /* End conditional branch: Acquire canonical owner. */
    if (!checked(fvm_load_memory(context, movie, sizeof(movie)), "load")) /* Copy/validate SWF. */
    {                                                                     /* Begin conditional branch: Copy/validate SWF. */
        goto cleanup;                                                     /* No output file has been created. */
    } /* End conditional branch: Copy/validate SWF. */
    result = fvm_swf_stream_sound_snapshot_open_pcm_bounded(                                          /* Bound preparation peak. */
                                                            context, 0, EXAMPLE_MIX_BUDGET, &source); /* Select the only root stream. */
    if (!checked(result, "stream"))                                                                   /* Prepare PCM before native publication. */
    {                                                                                                 /* Begin conditional branch: Prepare PCM before native publication. */
        goto cleanup;                                                                                 /* Release partially acquired VM ownership. */
    } /* End conditional branch: Prepare PCM before native publication. */
    geometry.struct_size = sizeof(geometry);                                         /* Match the current native configuration ABI. */
    geometry.sample_rate = EXAMPLE_MIX_RATE;                                         /* Share the explicit stream output clock. */
    geometry.channels = EXAMPLE_MIX_CHANNELS;                                        /* Match interleaved caller storage. */
    geometry.voice_capacity = EXAMPLE_MIX_VOICES;                                    /* Reserve exactly the two prepared events. */
    geometry.flags = FVM_SWF_SOUND_MIXER_CONVERT;                                    /* Convert native source rates/channels explicitly. */
    if (!checked(fvm_swf_sound_mixer_create_configured(&geometry, &mixer), "mixer")) /* Reserve voices. */
    {                                                                                /* Begin conditional branch: Reserve voices. */
        goto cleanup;                                                                /* No file side effect occurs on mixer failure. */
    } /* End conditional branch: Reserve voices. */
    for (event = 0; event < EXAMPLE_MIX_VOICES; ++event)                                                                     /* This fixture schedules both starts at frame one. */
    {                                                                                                                        /* Tracked mode delays adoption until the empty queue clock and ID table exist. */
        result = identity_mode ? fvm_swf_sound_voice_open_bounded(context, event, EXAMPLE_MIX_VOICE_BYTES, &prepared[event]) /* Bound each temporary native voice snapshot independently. */
                               : fvm_swf_sound_mixer_submit(mixer, context, event);                                          /* Existing modes retain direct owner-side admission. */
        if (!checked(result, "event preparation"))                                                                           /* Condition for this action: Cleanup releases any caller-owned prepared handle. */
        {                                                                                                                    /* Begin conditional branch: Condition for this action: Cleanup releases any caller-owned prepared handle. */
            goto cleanup;                                                                                                    /* Cleanup releases any caller-owned prepared handle. */
        } /* End conditional branch: Condition for this action: Cleanup releases any caller-owned prepared handle. */
    } /* No source VM pointer is retained by native voices. */
    result = fvm_swf_stream_converter_init(                                                             /* Borrow source without converter allocation. */
                                           &converter, source, EXAMPLE_MIX_RATE, EXAMPLE_MIX_CHANNELS); /* Match the mixer clock. */
    if (!checked(result, "converter"))                                                                  /* Condition for this action: Preserve source/geometry errors. */
    {                                                                                                   /* Begin conditional branch: Condition for this action: Preserve source/geometry errors. */
        goto cleanup;                                                                                   /* Preserve source/geometry errors. */
    } /* End conditional branch: Condition for this action: Preserve source/geometry errors. */
    if (queue_mode)                                                                                                                               /* Prepare adapter storage before source teardown and native-only production. */
    {                                                                                                                                             /* Bind one exclusive software FIFO to the configured event mixer. */
        memset(&backend, 0, sizeof(backend));                                                                                                     /* Initialize the complete callback descriptor. */
        backend.struct_size = sizeof(backend);                                                                                                    /* Match the current native callback ABI. */
        backend.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;                                                                                        /* Select the supported table version. */
        backend.sample_rate = EXAMPLE_MIX_RATE;                                                                                                   /* Match converter and event clocks. */
        backend.channels = EXAMPLE_MIX_CHANNELS;                                                                                                  /* Match every caller/native frame. */
        backend.userdata = &queue;                                                                                                                /* Borrow stable stack ownership until adapter close. */
        backend.write = queue_write;                                                                                                              /* Native copy/observe contract. */
        backend.queued = queue_count;                                                                                                             /* Native copy/observe contract. */
        backend.pause = queue_pause;                                                                                                              /* Explicit native controls. */
        backend.clear = queue_clear;                                                                                                              /* Explicit native controls. */
        if (bounded_mode)                                                                                                                         /* Plan the exact fixed adapter requests before acquiring them. */
        {                                                                                                                                         /* This does not include borrowed mixer, voices or software FIFO memory. */
            if (!checked(fvm_swf_sound_mix_queue_storage_required(EXAMPLE_MIX_CHANNELS, EXAMPLE_MIX_BLOCK, &queue_budget), "mix queue required")) /* Condition for this action: No allocation, callback or VM access. */
            {                                                                                                                                     /* Begin conditional branch: Condition for this action: No allocation, callback or VM access. */
                goto cleanup;                                                                                                                     /* No allocation, callback or VM access. */
            } /* End conditional branch: Condition for this action: No allocation, callback or VM access. */
            result = fvm_swf_sound_mix_queue_create_bounded(mixer, &backend, EXAMPLE_MIX_BLOCK, queue_budget, &mix_queue, &adapted); /* Exact-fit native acquisition. */
        } /* An insufficient budget would fail before any adapter allocation. */
        else                                                                                                   /* Existing modes retain unrestricted adapter acquisition. */
        {                                                                                                      /* Begin conditional branch: Existing modes retain unrestricted adapter acquisition. */
            result = fvm_swf_sound_mix_queue_create(mixer, &backend, EXAMPLE_MIX_BLOCK, &mix_queue, &adapted); /* Existing modes retain unrestricted adapter acquisition. */
        } /* End conditional branch: Existing modes retain unrestricted adapter acquisition. */
        if (!checked(result, "mix queue")) /* Condition for this action: Release earlier source/mixer on failure. */
        {                                  /* Begin conditional branch: Condition for this action: Release earlier source/mixer on failure. */
            goto cleanup;                  /* Release earlier source/mixer on failure. */
        } /* End conditional branch: Condition for this action: Release earlier source/mixer on failure. */
        if (bounded_mode)                                                                                 /* Observe portable requested bytes rather than claiming process RSS. */
        {                                                                                                 /* Require the documented storage descriptor size. */
            storage.struct_size = sizeof(storage);                                                        /* Preserve native ABI validation. */
            if (!checked(fvm_swf_sound_mix_queue_storage_info(mix_queue, &storage), "mix queue storage")) /* Condition for this action: No backend observation occurs. */
            {                                                                                             /* Begin conditional branch: Condition for this action: No backend observation occurs. */
                goto cleanup;                                                                             /* No backend observation occurs. */
            } /* End conditional branch: Condition for this action: No backend observation occurs. */
            if (storage.retained_bytes != queue_budget) /* Condition for this action: Predict and observe the same fixed requests. */
            {                                           /* Begin conditional branch: Condition for this action: Predict and observe the same fixed requests. */
                goto cleanup;                           /* Predict and observe the same fixed requests. */
            } /* End conditional branch: Condition for this action: Predict and observe the same fixed requests. */
            printf("Mix queue buffers=%lu fixed=%lu retained=%lu\n", (unsigned long)storage.buffer_bytes, (unsigned long)storage.fixed_bytes, (unsigned long)storage.retained_bytes); /* Sizes are platform-dependent except for the fixed PCM arrays. */
        } /* Native hot-path buffers never grow. */
    } /* End owner-side preparation. */
    if (clock_mode && !checked(fvm_swf_sound_mix_queue_clock_begin(mix_queue), "shared clock begin")) /* Condition for this action: Explicitly require an empty real FIFO before counting accepted frames. */
    {                                                                                                 /* Begin conditional branch: Condition for this action: Explicitly require an empty real FIFO before counting accepted frames. */
        goto cleanup;                                                                                 /* Explicitly require an empty real FIFO before counting accepted frames. */
    } /* End conditional branch: Condition for this action: Explicitly require an empty real FIFO before counting accepted frames. */
    if (identity_mode)                                                                                                                                                /* Preallocate the optional ID table before native output starts. */
    {                                                                                                                                                                 /* Keep adapter table storage separate from mixer/voice/queue budgets. */
        if (!checked(fvm_swf_sound_mix_queue_storage_required_for_modes(EXAMPLE_MIX_CHANNELS, EXAMPLE_MIX_BLOCK, 1, origins_mode, &queue_budget), "tracked storage")) /* Condition for this action: Predict owner, PCM and optional deadlines with checked native arithmetic. */
        {                                                                                                                                                             /* Begin conditional branch: Condition for this action: Predict owner, PCM and optional deadlines with checked native arithmetic. */
            goto cleanup;                                                                                                                                             /* Predict owner, PCM and optional deadlines with checked native arithmetic. */
        } /* End conditional branch: Condition for this action: Predict owner, PCM and optional deadlines with checked native arithmetic. */
        if (!checked(fvm_swf_sound_mix_queue_set_identity_tracking(mix_queue, 1, queue_budget), "identity tracking")) /* Condition for this action: Require zero epoch and no voices before enabling. */
        {                                                                                                             /* Begin conditional branch: Condition for this action: Require zero epoch and no voices before enabling. */
            goto cleanup;                                                                                             /* Require zero epoch and no voices before enabling. */
        } /* End conditional branch: Condition for this action: Require zero epoch and no voices before enabling. */
        if (origins_mode && !checked(fvm_swf_sound_mix_queue_set_origin_tracking(mix_queue, 1, queue_budget), "origin tracking")) /* Condition for this action: Allocate the fixed origin bitmap only at this empty shared epoch. */
        {                                                                                                                         /* Begin conditional branch: Condition for this action: Allocate the fixed origin bitmap only at this empty shared epoch. */
            goto cleanup;                                                                                                         /* Allocate the fixed origin bitmap only at this empty shared epoch. */
        } /* End conditional branch: Condition for this action: Allocate the fixed origin bitmap only at this empty shared epoch. */
        for (event = 0; event < EXAMPLE_MIX_VOICES; ++event)                                                                                      /* Adopt every start through the tracked queue. */
        {                                                                                                                                         /* Begin loop body: Adopt every start through the tracked queue. */
            if (!checked(fvm_swf_sound_mix_queue_submit_prepared_bounded(mix_queue, &prepared[event], EXAMPLE_MIX_EVENT_BYTES), "tracked event")) /* Condition for this action: Success clears the caller handle and registers its exact end. */
            {                                                                                                                                     /* Begin conditional branch: Condition for this action: Success clears the caller handle and registers its exact end. */
                goto cleanup;                                                                                                                     /* Success clears the caller handle and registers its exact end. */
            } /* End conditional branch: Condition for this action: Success clears the caller handle and registers its exact end. */
        } /* End loop body: Adopt every start through the tracked queue. */
    } /* All later ID controls also use this tracked owner. */
    if (gated_mode && !checked(fvm_swf_stream_output_init(&gated_output, &converter, &adapted, pcm, sizeof(pcm) / sizeof(short), EXAMPLE_MIX_BLOCK, EXAMPLE_MIX_QUEUE_FRAMES), "gated producer")) /* Condition for this action: Initialize against the exact tracked backend with an empty FIFO. */
    {                                                                                                                                                                                             /* Begin conditional branch: Condition for this action: Initialize against the exact tracked backend with an empty FIFO. */
        goto cleanup;                                                                                                                                                                             /* Initialize against the exact tracked backend with an empty FIFO. */
    } /* End conditional branch: Condition for this action: Initialize against the exact tracked backend with an empty FIFO. */
    fvm_context_destroy(context);  /* [GC:] End canonical ownership before the native loop. */
    context = 0;                   /* [GC:] Prevent cleanup from destroying the former context again. */
    fvm_runtime_destroy(runtime);  /* Independent native sources no longer need their runtime. */
    runtime = 0;                   /* Prevent duplicate runtime destruction during fallback cleanup. */
    output = fopen(argv[1], "wb"); /* Replace only the explicit file after complete preparation. */
    if (!output)                   /* Surface the actual file-open error after native preparation. */
    {                              /* Preserve independent cleanup even when the selected directory is missing. */
        perror(argv[1]);           /* Report the explicit path and native failure reason. */
        goto cleanup;              /* No retry pretends to undo file side effects. */
    } /* Finish failed native acquisition. */
    if (headroom_mode || finite_mode)                                                                            /* Demonstrate partial headroom without assigning fake stream time. */
    {                                                                                                            /* Base production is explicitly quiescent for the complete event-only export. */
        FVM_SWFStreamConverter held = converter;                                                                 /* Preserve an independent source-position witness. */
        size_t horizon = 0;                                                                                      /* Copy the bounded live duration before any rejected render advances voices. */
        while (total < EXAMPLE_MIX_EVENT_FRAMES)                                                                 /* Both event voices finish after fourteen converted frames. */
        {                                                                                                        /* Increase the target by only one frame while retaining the larger maximum block. */
            queue.reject = 1;                                                                                    /* Reject the new short mixture once without copying it. */
            if (!checked(fvm_swf_sound_mixer_active_frames(mixer, EXAMPLE_MIX_BLOCK, &horizon), "live horizon")) /* Condition for this action: Query without consuming fractional lookahead or PCM. */
            {                                                                                                    /* Begin conditional branch: Condition for this action: Query without consuming fractional lookahead or PCM. */
                goto cleanup;                                                                                    /* Query without consuming fractional lookahead or PCM. */
            } /* End conditional branch: Condition for this action: Query without consuming fractional lookahead or PCM. */
            result = finite_mode ? fvm_swf_sound_mix_queue_pump_tail_finite(mix_queue, EXAMPLE_MIX_BLOCK, EXAMPLE_MIX_QUEUE_FRAMES, &accepted) /* Trim fresh final work to the actual remaining voice horizon. */
                                 : fvm_swf_sound_mix_queue_pump_tail_up_to(mix_queue, EXAMPLE_MIX_BLOCK, total + 1, &accepted);                /* Available headroom selects exactly one frame. */
            if (result != FVM_NOT_FOUND || accepted)                                                                                           /* Condition for this action: This controlled rejection retains an indivisible short TAIL. */
            {                                                                                                                                  /* Begin conditional branch: Condition for this action: This controlled rejection retains an indivisible short TAIL. */
                goto cleanup;                                                                                                                  /* This controlled rejection retains an indivisible short TAIL. */
            } /* End conditional branch: Condition for this action: This controlled rejection retains an indivisible short TAIL. */
            result = finite_mode ? fvm_swf_sound_mix_queue_pump_tail_finite(mix_queue, EXAMPLE_MIX_BLOCK, EXAMPLE_MIX_QUEUE_FRAMES, &accepted) /* Retry the original finite block, including its short final size. */
                                 : fvm_swf_sound_mix_queue_pump_tail_up_to(mix_queue, EXAMPLE_MIX_BLOCK, EXAMPLE_MIX_QUEUE_FRAMES, &accepted); /* A larger target must resend the same short block. */
            if (!checked(result, "finite/adaptive retry"))                                                                                     /* Condition for this action: Require actual all-or-none FIFO acceptance. */
            {                                                                                                                                  /* Begin conditional branch: Condition for this action: Require actual all-or-none FIFO acceptance. */
                goto cleanup;                                                                                                                  /* Require actual all-or-none FIFO acceptance. */
            } /* End conditional branch: Condition for this action: Require actual all-or-none FIFO acceptance. */
            if (accepted != (finite_mode ? horizon : 1)) /* Condition for this action: Reject accidental growth, truncation or re-rendering of the retained block. */
            {                                            /* Begin conditional branch: Condition for this action: Reject accidental growth, truncation or re-rendering of the retained block. */
                goto cleanup;                            /* Reject accidental growth, truncation or re-rendering of the retained block. */
            } /* End conditional branch: Condition for this action: Reject accidental growth, truncation or re-rendering of the retained block. */
            total += accepted;                           /* Advance only after the real FIFO acknowledges copying. */
            if (memcmp(&held, &converter, sizeof(held))) /* Condition for this action: Event-only work never advances the base source. */
            {                                            /* Begin conditional branch: Condition for this action: Event-only work never advances the base source. */
                goto cleanup;                            /* Event-only work never advances the base source. */
            } /* End conditional branch: Condition for this action: Event-only work never advances the base source. */
        } /* No final padding is necessary with this deliberately one-frame schedule. */
        if (fvm_swf_sound_mix_queue_pump_tail_up_to(mix_queue, EXAMPLE_MIX_BLOCK, EXAMPLE_MIX_QUEUE_FRAMES, &accepted) != FVM_NOT_FOUND || accepted) /* Condition for this action: Exhaustion cannot generate endless silence. */
        {                                                                                                                                            /* Begin conditional branch: Condition for this action: Exhaustion cannot generate endless silence. */
            goto cleanup;                                                                                                                            /* Exhaustion cannot generate endless silence. */
        } /* End conditional branch: Condition for this action: Exhaustion cannot generate endless silence. */
    } /* Common clock, ID, file-consumption and cleanup code remains shared. */
    if (gated_mode)                                                                                                                                           /* Advance events over zero base, then resume the same untouched converter. */
    {                                                                                                                                                         /* Keep every rejected block owned by its original producer. */
        FVM_SWFStreamConverter held = converter;                                                                                                              /* Copy only for an independent no-advance witness. */
        while (total < EXAMPLE_MIX_GATED_TOTAL)                                                                                                               /* Bound all native work and accepted storage. */
        {                                                                                                                                                     /* Authorize one known block and deliberately reject it once. */
            queue.reject = 1;                                                                                                                                 /* The actual backend accepts no bytes on this failure. */
            result = fvm_swf_stream_output_pump_mix_until(&gated_output, mix_queue, total < EXAMPLE_MIX_GATE_FRAMES ? 0 : EXAMPLE_MIX_OPEN_FRAME, &accepted); /* Gate zero holds base but permits events. */
            if (result != FVM_NOT_FOUND || accepted)                                                                                                          /* Condition for this action: This fixture's armed rejection is expected. */
            {                                                                                                                                                 /* Begin conditional branch: Condition for this action: This fixture's armed rejection is expected. */
                goto cleanup;                                                                                                                                 /* This fixture's armed rejection is expected. */
            } /* End conditional branch: Condition for this action: This fixture's armed rejection is expected. */
            if (!checked(fvm_swf_stream_output_pump_mix_until(&gated_output, mix_queue, EXAMPLE_MIX_OPEN_FRAME, &accepted), "gated retry")) /* Condition for this action: An opening gate cannot turn retained TAIL into BASE. */
            {                                                                                                                               /* Begin conditional branch: Condition for this action: An opening gate cannot turn retained TAIL into BASE. */
                goto cleanup;                                                                                                               /* An opening gate cannot turn retained TAIL into BASE. */
            } /* End conditional branch: Condition for this action: An opening gate cannot turn retained TAIL into BASE. */
            if (!accepted || accepted > EXAMPLE_MIX_GATED_TOTAL - total) /* Condition for this action: Require finite forward progress. */
            {                                                            /* Begin conditional branch: Condition for this action: Require finite forward progress. */
                goto cleanup;                                            /* Require finite forward progress. */
            } /* End conditional branch: Condition for this action: Require finite forward progress. */
            total += accepted;                                                               /* Count accepted event-only and base frames together. */
            if (total <= EXAMPLE_MIX_GATE_FRAMES && memcmp(&held, &converter, sizeof(held))) /* Condition for this action: Held event blocks must leave every converter field intact. */
            {                                                                                /* Begin conditional branch: Condition for this action: Held event blocks must leave every converter field intact. */
                goto cleanup;                                                                /* Held event blocks must leave every converter field intact. */
            } /* End conditional branch: Condition for this action: Held event blocks must leave every converter field intact. */
        } /* All source and voice frames now fit the explicit software FIFO. */
        if (fvm_swf_stream_output_pump_mix_until(&gated_output, mix_queue, EXAMPLE_MIX_OPEN_FRAME, &accepted) != FVM_NOT_FOUND || accepted) /* Condition for this action: Exhaustion must not enqueue extra silence. */
        {                                                                                                                                   /* Begin conditional branch: Condition for this action: Exhaustion must not enqueue extra silence. */
            goto cleanup;                                                                                                                   /* Exhaustion must not enqueue extra silence. */
        } /* End conditional branch: Condition for this action: Exhaustion must not enqueue extra silence. */
    } /* The common file consumer below runs only after every retry has been accepted. */
    while (!gated_mode && !headroom_mode && !finite_mode && total < EXAMPLE_MIX_TOTAL) /* Host chooses finite duration, including an event tail. */
    {                                                                                  /* Produce a new unmixed base exactly once for this output block. */
        count = EXAMPLE_MIX_TOTAL - total;                                             /* Limit the final block to the finite export budget. */
        if (count > EXAMPLE_MIX_BLOCK)                                                 /* Condition for this action: Preserve caller buffer bounds. */
        {                                                                              /* Begin conditional branch: Condition for this action: Preserve caller buffer bounds. */
            count = EXAMPLE_MIX_BLOCK;                                                 /* Preserve caller buffer bounds. */
        } /* End conditional branch: Condition for this action: Preserve caller buffer bounds. */
        memset(pcm, 0, sizeof(pcm));                                                              /* Explicit zero base serves only a host-authorized tail. */
        values = 0;                                                                               /* Explicit zero base serves only a host-authorized tail. */
        if (!eof)                                                                                 /* Do not consult the converter after its normal exhaustion. */
        {                                                                                         /* Read at most one native stream chunk without crossing a source block. */
            chunk.struct_size = sizeof(chunk);                                                    /* Validate copied chunk metadata size. */
            result = fvm_swf_stream_converter_next(                                               /* Convert only this bounded source slice. */
                                                   &converter, count, pcm,                        /* Retain caller storage for later native output. */
                                                   sizeof(pcm) / sizeof(short), &chunk, &values); /* Receive exact initialized values. */
            if (result == FVM_NOT_FOUND)                                                          /* Condition for this action: A tail now uses the explicit zero base. */
            {                                                                                     /* Begin conditional branch: Condition for this action: A tail now uses the explicit zero base. */
                eof = 1;                                                                          /* A tail now uses the explicit zero base. */
            } /* End conditional branch: Condition for this action: A tail now uses the explicit zero base. */
            else if (!checked(result, "stream read")) /* Never turn a decoding error into silence. */
            {                                         /* Begin conditional branch: Never turn a decoding error into silence. */
                goto cleanup;                         /* Never turn a decoding error into silence. */
            } /* End conditional branch: Never turn a decoding error into silence. */
            else if (!values) /* Empty markers advance source metadata, not voice time. */
            {                 /* Begin conditional branch: Empty markers advance source metadata, not voice time. */
                continue;     /* Empty markers advance source metadata, not voice time. */
            } /* End conditional branch: Empty markers advance source metadata, not voice time. */
            else                                       /* Mix only initialized converted frames. */
            {                                          /* Begin conditional branch: Mix only initialized converted frames. */
                count = values / EXAMPLE_MIX_CHANNELS; /* Mix only initialized converted frames. */
            } /* End conditional branch: Mix only initialized converted frames. */
        } /* Finish one source observation. */
        if (queue_mode)                                                                                                   /* Queue accepts only the retained mixture, never a second mixer advance. */
        {                                                                                                                 /* Deliberately reject each new base before accepting its identical retry. */
            queue.reject = 1;                                                                                             /* Arm the native consumer before the first write attempt. */
            result = tail_mode && eof ? fvm_swf_sound_mix_queue_pump_tail(mix_queue, count, EXAMPLE_MIX_TOTAL, &accepted) /* Render a zero-base event block only after converter exhaustion. */
                                      : adapted.write(adapted.userdata, pcm, count);                                      /* Base input keeps its original-byte retry contract. */
            if (result != FVM_NOT_FOUND)                                                                                  /* Condition for this action: Require actual controlled backpressure. */
            {                                                                                                             /* Begin conditional branch: Condition for this action: Require actual controlled backpressure. */
                goto cleanup;                                                                                             /* Require actual controlled backpressure. */
            } /* End conditional branch: Condition for this action: Require actual controlled backpressure. */
            pending.struct_size = sizeof(pending);                                      /* Match native retry observation geometry. */
            if (!checked(fvm_swf_sound_mix_queue_info(mix_queue, &pending), "pending")) /* Condition for this action: No device poll. */
            {                                                                           /* Begin conditional branch: Condition for this action: No device poll. */
                goto cleanup;                                                           /* No device poll. */
            } /* End conditional branch: Condition for this action: No device poll. */
            ownership.struct_size = sizeof(ownership);                                              /* Copy retry origin separately from the legacy Info ABI. */
            if (!checked(fvm_swf_sound_mix_queue_pending(mix_queue, &ownership), "pending origin")) /* Condition for this action: No queue callback or voice movement. */
            {                                                                                       /* Begin conditional branch: Condition for this action: No queue callback or voice movement. */
                goto cleanup;                                                                       /* No queue callback or voice movement. */
            } /* End conditional branch: Condition for this action: No queue callback or voice movement. */
            if (ownership.frames != count || ownership.origin != (tail_mode && eof ? FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL : FVM_SWF_SOUND_MIX_QUEUE_PENDING_BASE)) /* Condition for this action: Never let one producer steal the other's retry. */
            {                                                                                                                                                      /* Begin conditional branch: Condition for this action: Never let one producer steal the other's retry. */
                goto cleanup;                                                                                                                                      /* Never let one producer steal the other's retry. */
            } /* End conditional branch: Condition for this action: Never let one producer steal the other's retry. */
            if (pending.pending_frames != count) /* Condition for this action: The adapter owns exactly this retry. */
            {                                    /* Begin conditional branch: Condition for this action: The adapter owns exactly this retry. */
                goto cleanup;                    /* The adapter owns exactly this retry. */
            } /* End conditional branch: Condition for this action: The adapter owns exactly this retry. */
            result = tail_mode && eof ? fvm_swf_sound_mix_queue_pump_tail(mix_queue, count, EXAMPLE_MIX_TOTAL, &accepted) /* Resend retained tail bytes without another voice advance. */
                                      : adapted.write(adapted.userdata, pcm, count);                                      /* Base retry reuses the original bytes unchanged. */
            if (!checked(result, "retry"))                                                                                /* Condition for this action: Never count a rejected block as accepted time. */
            {                                                                                                             /* Begin conditional branch: Condition for this action: Never count a rejected block as accepted time. */
                goto cleanup;                                                                                             /* Never count a rejected block as accepted time. */
            } /* End conditional branch: Condition for this action: Never count a rejected block as accepted time. */
            if (tail_mode && eof && accepted != count) /* Condition for this action: Require the complete authorized tail block. */
            {                                          /* Begin conditional branch: Condition for this action: Require the complete authorized tail block. */
                goto cleanup;                          /* Require the complete authorized tail block. */
            } /* End conditional branch: Condition for this action: Require the complete authorized tail block. */
        } /* Retained mixed bytes have now moved to the native FIFO. */
        else                                                                                          /* The original direct-file demonstration still uses explicit in-place mixing. */
        {                                                                                             /* A file write failure remains terminal rather than a queue retry. */
            result = fvm_swf_sound_mixer_add(                                                         /* Sum base and all voices before clipping once. */
                                             mixer, count, pcm, sizeof(pcm) / sizeof(short), &mixed); /* Overwrite owned PCM. */
            if (!checked(result, "mix"))                                                              /* Condition for this action: Preserve ordinary capacity/owner errors. */
            {                                                                                         /* Begin conditional branch: Condition for this action: Preserve ordinary capacity/owner errors. */
                goto cleanup;                                                                         /* Preserve ordinary capacity/owner errors. */
            } /* End conditional branch: Condition for this action: Preserve ordinary capacity/owner errors. */
            if (mixed != count * EXAMPLE_MIX_CHANNELS) /* Condition for this action: Require complete mixing. */
            {                                          /* Begin conditional branch: Condition for this action: Require complete mixing. */
                goto cleanup;                          /* Require complete mixing. */
            } /* End conditional branch: Condition for this action: Require complete mixing. */
            if (fwrite(pcm, sizeof(short), mixed, output) != mixed) /* Condition for this action: Terminal file failure. */
            {                                                       /* Begin conditional branch: Condition for this action: Terminal file failure. */
                goto cleanup;                                       /* Terminal file failure. */
            } /* End conditional branch: Condition for this action: Terminal file failure. */
        } /* End one explicit output policy. */
        total += count; /* Advance the host output clock only after a complete write. */
    } /* End bounded serial export; a retrying backend must retain pcm instead of mixing again. */
    if (queue_mode)                                                                                                                     /* The file is a separate final consumer, outside queue retry semantics. */
    {                                                                                                                                   /* Consume exactly the acknowledged native FIFO prefix once. */
        if (tail_mode && !gated_mode && fvm_swf_sound_mix_queue_pump_tail(mix_queue, 1, EXAMPLE_MIX_TOTAL, &accepted) != FVM_NOT_FOUND) /* Condition for this action: Exhausted voices must not enqueue more silence. */
        {                                                                                                                               /* Begin conditional branch: Condition for this action: Exhausted voices must not enqueue more silence. */
            goto cleanup;                                                                                                               /* Exhausted voices must not enqueue more silence. */
        } /* End conditional branch: Condition for this action: Exhausted voices must not enqueue more silence. */
        if (tail_mode && accepted) /* Condition for this action: Exhaustion clears accepted without draining the FIFO. */
        {                          /* Begin conditional branch: Condition for this action: Exhaustion clears accepted without draining the FIFO. */
            goto cleanup;          /* Exhaustion clears accepted without draining the FIFO. */
        } /* End conditional branch: Condition for this action: Exhaustion clears accepted without draining the FIFO. */
        if (queue.frames != ((headroom_mode || finite_mode) ? EXAMPLE_MIX_EVENT_FRAMES : gated_mode ? EXAMPLE_MIX_GATED_TOTAL /* Condition for this action: Reject incomplete native export. */
                                                                                                    : EXAMPLE_MIX_TOTAL))     /* Reject incomplete native export. */
        {                                                                                                                     /* Begin scope: Reject incomplete native export. */
            goto cleanup;                                                                                                     /* Reject incomplete native export. */
        } /* End scope: Reject incomplete native export. */
        if (clock_mode)                                                                                        /* All accepted base and tail blocks are still waiting in this software FIFO. */
        {                                                                                                      /* Read a fresh validated observation without changing voice time. */
            shared_clock.struct_size = sizeof(shared_clock);                                                   /* Required public clock descriptor size. */
            if (!checked(fvm_swf_sound_mix_queue_clock_info(mix_queue, &shared_clock), "shared queued clock")) /* Condition for this action: No DAC position is claimed. */
            {                                                                                                  /* Begin conditional branch: Condition for this action: No DAC position is claimed. */
                goto cleanup;                                                                                  /* No DAC position is claimed. */
            } /* End conditional branch: Condition for this action: No DAC position is claimed. */
            printf("Shared clock submitted=%.0f queued=%lu played=%.0f\n", shared_clock.submitted_frames, (unsigned long)shared_clock.queued_frames, shared_clock.played_frames); /* Expected 16, 16, zero before software consumption. */
        } /* Clock snapshots are owned values. */
        if (identity_mode)                                                                                                     /* Mixer retirement alone cannot expire queued sound identity. */
        {                                                                                                                      /* ID8 is the longer event in the embedded SWF. */
            identity.struct_size = sizeof(identity);                                                                           /* Require the exact copied observation ABI. */
            if (!checked(fvm_swf_sound_mix_queue_identity_info(mix_queue, EXAMPLE_MIX_LONG_ID, &identity), "queued identity")) /* Condition for this action: Observe real FIFO before publishing its deadline. */
            {                                                                                                                  /* Begin conditional branch: Condition for this action: Observe real FIFO before publishing its deadline. */
                goto cleanup;                                                                                                  /* Observe real FIFO before publishing its deadline. */
            } /* End conditional branch: Condition for this action: Observe real FIFO before publishing its deadline. */
            printf("Identity id=%u end=%.0f played=%.0f active=%d\n", identity.sound_id, identity.end_frame, identity.played_frames, identity.active); /* Expected ID8 end14 played0 active1 in both tracked modes. */
        } /* This reports dequeue registration, not physical audibility. */
        if (origins_mode)                                                                                       /* Observe accepted categories before any software FIFO consumption. */
        {                                                                                                       /* Failed renders never entered this category ledger. */
            paired.struct_size = sizeof(paired);                                                                /* Nested descriptors are initialized by the paired query. */
            if (!checked(fvm_swf_stream_output_mix_info(&gated_output, mix_queue, &paired), "queued snapshot")) /* Condition for this action: One FIFO observation captures stream metadata and both categories. */
            {                                                                                                   /* Begin conditional branch: Condition for this action: One FIFO observation captures stream metadata and both categories. */
                goto cleanup;                                                                                   /* One FIFO observation captures stream metadata and both categories. */
            } /* End conditional branch: Condition for this action: One FIFO observation captures stream metadata and both categories. */
            origin = paired.origins;                                                                                                                                             /* Retain the owned category value after releasing the query guard. */
            printf("Snapshot submitted=%.0f played=%.0f queued=%lu\n", paired.stream.submitted_frames, paired.stream.played_frames, (unsigned long)paired.stream.queued_frames); /* Expected seventeen accepted and queued frames before software drain. */
            printf("Origins base=%.0f/%.0f tail=%.0f/%.0f\n", origin.base_submitted_frames, origin.base_played_frames, origin.tail_submitted_frames, origin.tail_played_frames); /* Expected base9/0 and tail8/0 while all seventeen frames remain queued. */
        } /* Category sums agree with the shared submitted/dequeued clock. */
        mixed = queue.frames * EXAMPLE_MIX_CHANNELS;                  /* Convert frames to scalar S16 values. */
        if (fwrite(queue.pcm, sizeof(short), mixed, output) != mixed) /* Condition for this action: Terminal I/O. */
        {                                                             /* Begin conditional branch: Condition for this action: Terminal I/O. */
            goto cleanup;                                             /* Terminal I/O. */
        } /* End conditional branch: Condition for this action: Terminal I/O. */
        if (clock_mode)                                                                                         /* The successful file write is the explicit software FIFO consumer. */
        {                                                                                                       /* Only acknowledge drain after every queued scalar was copied. */
            queue.frames = 0;                                                                                   /* Publish the actual software dequeue; this is not a device playhead. */
            if (!checked(fvm_swf_sound_mix_queue_clock_info(mix_queue, &shared_clock), "shared drained clock")) /* Condition for this action: Observe submitted-minus-backlog after consumption. */
            {                                                                                                   /* Begin conditional branch: Condition for this action: Observe submitted-minus-backlog after consumption. */
                goto cleanup;                                                                                   /* Observe submitted-minus-backlog after consumption. */
            } /* End conditional branch: Condition for this action: Observe submitted-minus-backlog after consumption. */
            printf("Shared clock submitted=%.0f queued=%lu played=%.0f seconds=%.9f\n", shared_clock.submitted_frames, (unsigned long)shared_clock.queued_frames, shared_clock.played_frames, shared_clock.seconds); /* The count is 16 normally or 17 when gated; seconds uses the same 48000 Hz. */
            if (identity_mode)                                                                                                                                                                                       /* Observe expiry only after the software consumer acknowledged drain. */
            {                                                                                                                                                                                                        /* Cancel the expired registration and disable at a drained safe point. */
                if (!checked(fvm_swf_sound_mix_queue_identity_info(mix_queue, EXAMPLE_MIX_LONG_ID, &identity), "drained identity"))                                                                                  /* Condition for this action: Retain the historical end in this copied snapshot. */
                {                                                                                                                                                                                                    /* Begin conditional branch: Condition for this action: Retain the historical end in this copied snapshot. */
                    goto cleanup;                                                                                                                                                                                    /* Retain the historical end in this copied snapshot. */
                } /* End conditional branch: Condition for this action: Retain the historical end in this copied snapshot. */
                printf("Identity id=%u end=%.0f played=%.0f active=%d\n", identity.sound_id, identity.end_frame, identity.played_frames, identity.active); /* Expected end14, played16 normally or played17 when gated, active0. */
                if (!checked(fvm_swf_sound_mix_queue_stop_all(mix_queue), "cancel identities"))                                                            /* Condition for this action: Cancel every future event registration without rewriting accepted PCM. */
                {                                                                                                                                          /* Begin conditional branch: Condition for this action: Cancel every future event registration without rewriting accepted PCM. */
                    goto cleanup;                                                                                                                          /* Cancel every future event registration without rewriting accepted PCM. */
                } /* End conditional branch: Condition for this action: Cancel every future event registration without rewriting accepted PCM. */
                if (!checked(fvm_swf_sound_mix_queue_set_identity_tracking(mix_queue, 0, 0), "disable identities")) /* Condition for this action: Free optional deadlines only after voices/retries/FIFO have drained. */
                {                                                                                                   /* Begin conditional branch: Condition for this action: Free optional deadlines only after voices/retries/FIFO have drained. */
                    goto cleanup;                                                                                   /* Free optional deadlines only after voices/retries/FIFO have drained. */
                } /* End conditional branch: Condition for this action: Free optional deadlines only after voices/retries/FIFO have drained. */
            } /* Clock end is now permitted. */
            if (origins_mode)                                                                                        /* Observe category drain before releasing the optional bookkeeping mode. */
            {                                                                                                        /* ID tracking is already disabled at its independent safe point. */
                if (!checked(fvm_swf_stream_output_mix_info(&gated_output, mix_queue, &paired), "drained snapshot")) /* Condition for this action: Observe the acknowledged software drain in both clocks at once. */
                {                                                                                                    /* Begin conditional branch: Condition for this action: Observe the acknowledged software drain in both clocks at once. */
                    goto cleanup;                                                                                    /* Observe the acknowledged software drain in both clocks at once. */
                } /* End conditional branch: Condition for this action: Observe the acknowledged software drain in both clocks at once. */
                origin = paired.origins;                                                                                                                                             /* Category counters belong to this same zero-backlog observation. */
                printf("Snapshot submitted=%.0f played=%.0f queued=%lu\n", paired.stream.submitted_frames, paired.stream.played_frames, (unsigned long)paired.stream.queued_frames); /* Expected seventeen accepted and dequeued frames. */
                printf("Origins base=%.0f/%.0f tail=%.0f/%.0f\n", origin.base_submitted_frames, origin.base_played_frames, origin.tail_submitted_frames, origin.tail_played_frames); /* Expected base9/9 and tail8/8. */
                if (!checked(fvm_swf_sound_mix_queue_set_origin_tracking(mix_queue, 0, 0), "disable origins"))                                                                       /* Condition for this action: Free the bitmap only after no retry or FIFO remains. */
                {                                                                                                                                                                    /* Begin conditional branch: Condition for this action: Free the bitmap only after no retry or FIFO remains. */
                    goto cleanup;                                                                                                                                                    /* Free the bitmap only after no retry or FIFO remains. */
                } /* End conditional branch: Condition for this action: Free the bitmap only after no retry or FIFO remains. */
            } /* Clock end is permitted once both optional modes have been released. */
            if (!checked(fvm_swf_sound_mix_queue_clock_end(mix_queue), "shared clock end")) /* Condition for this action: End only after retry ownership and FIFO have drained. */
            {                                                                               /* Begin conditional branch: Condition for this action: End only after retry ownership and FIFO have drained. */
                goto cleanup;                                                               /* End only after retry ownership and FIFO have drained. */
            } /* End conditional branch: Condition for this action: End only after retry ownership and FIFO have drained. */
        } /* The adapter remains alive until centralized cleanup. */
        printf("Queue retained %lu rejected blocks.\n", (unsigned long)queue.rejected); /* Report real callbacks. */
    } /* No file consumer pretends to be an atomic queue backend. */
    status = fclose(output) == 0 ? 0 : 1;                                             /* Retain the actual final flush/close result. */
    output = 0;                                                                       /* A failed fclose still ends FILE ownership. */
    if (!status)                                                                      /* Condition for this action: Report the actual completed finite export. */
    {                                                                                 /* Begin conditional branch: Condition for this action: Report the actual completed finite export. */
        printf("Mixed %lu frames after source VM teardown.\n", (unsigned long)total); /* Report the actual completed finite export. */
    } /* End conditional branch: Condition for this action: Report the actual completed finite export. */
cleanup:                                                 /* Centralized cleanup also handles partial source preparation. */
    for (event = 0; event < EXAMPLE_MIX_VOICES; ++event) /* Release only handles not consumed by successful tracked adoption. */
    {                                                    /* Begin loop body: Release only handles not consumed by successful tracked adoption. */
        fvm_swf_sound_voice_close(prepared[event]);      /* Release only handles not consumed by successful tracked adoption. */
    } /* End loop body: Release only handles not consumed by successful tracked adoption. */
    if (output)         /* Condition for this action: Close any file acquired before a later native error. */
    {                   /* Begin conditional branch: Condition for this action: Close any file acquired before a later native error. */
        fclose(output); /* Close any file acquired before a later native error. */
    } /* End conditional branch: Condition for this action: Close any file acquired before a later native error. */
    fvm_swf_sound_mix_queue_close(mix_queue);    /* Quiesce wrapped callbacks before borrowed owners. */
    fvm_swf_sound_mixer_close(mixer);            /* Release all remaining event voices; null is accepted. */
    fvm_swf_stream_sound_snapshot_close(source); /* Release independent source after converter use ends. */
    if (context)                                 /* Condition for this action: [GC:] Release source context if preparation failed. */
    {                                            /* Begin conditional branch: Condition for this action: [GC:] Release source context if preparation failed. */
        fvm_context_destroy(context);            /* [GC:] Release source context if preparation failed. */
    } /* End conditional branch: Condition for this action: [GC:] Release source context if preparation failed. */
    if (runtime)                      /* Condition for this action: Release runtime last on every preparation failure. */
    {                                 /* Begin conditional branch: Condition for this action: Release runtime last on every preparation failure. */
        fvm_runtime_destroy(runtime); /* Release runtime last on every preparation failure. */
    } /* End conditional branch: Condition for this action: Release runtime last on every preparation failure. */
    return status; /* Let C/C++ hosts observe ordinary success or failure. */
} /* End the complete C89 example. */
