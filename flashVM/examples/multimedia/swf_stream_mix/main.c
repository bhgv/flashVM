/* Tutorial: mix real SWF stream PCM and StartSound snapshots after source VM teardown. */
#include "flashvm/swf_stream_convert.h" /* Read native stream blocks on an explicit output clock. */
#include "flashvm/swf_sound_mix_queue.h" /* Retain mixed bytes through explicit queue rejection. */
#include "flashvm/swf_sound_mixer.h" /* Add event voices before a single saturation pass. */
#include "example_constants.h" /* Keep tutorial geometry distinct from wire constants. */
#include <stdio.h> /* Write only the explicitly selected PCM output file. */
#include <string.h> /* Initialize zero base for an explicit event-only tail. */
/* Report recoverable API failures without tutorial assertions. */
static int checked(FVM_Result result, const char *operation) /* All error paths retain cleanup. */
{ /* Begin result translation. */
    if (result == FVM_OK) return 1; /* Continue only after acknowledged success. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Retain the exact result code. */
    return 0; /* Request centralized cleanup. */
} /* Finish result translation. */
/* A bounded real native FIFO accepts all frames or none; file consumption happens later. */
typedef struct ExampleQueue /* Keep all callback userdata alive until adapter close. */
{ /* Begin explicit queue ownership. */
    short pcm[EXAMPLE_MIX_TOTAL * EXAMPLE_MIX_CHANNELS]; /* Own the finite accepted-frame store. */
    size_t frames; /* Current initialized complete-frame count. */
    size_t rejected; /* Count deliberate transient rejections before any sample copy. */
    int reject; /* Owner arms one controlled rejection for the next input block. */
    int paused; /* Last acknowledged software pause state; no device is implied. */
} ExampleQueue; /* Finish fixed-capacity native state. */
/* An accepted write copies everything; a rejected write changes no FIFO bytes or count. */
static FVM_Result queue_write(void *raw, const short *pcm, size_t frames) /* Serial consumer. */
{ /* Begin atomic software acceptance. */
    ExampleQueue *queue = (ExampleQueue *)raw; /* Recover bounded native ownership. */
    if (queue->reject) /* Reject before touching the actual FIFO. */
    { /* One deliberate transient failure must not remix event voices. */
        queue->reject = 0; ++queue->rejected; /* Record one rejection without consuming samples. */
        return FVM_NOT_FOUND; /* No frames were accepted; identical retry is permitted. */
    } /* End deliberate rejection. */
    if (frames > EXAMPLE_MIX_TOTAL - queue->frames) return FVM_OUT_OF_MEMORY; /* Preserve bounds. */
    memcpy(queue->pcm + queue->frames * EXAMPLE_MIX_CHANNELS, pcm, /* Copy accepted source bytes. */
        frames * EXAMPLE_MIX_CHANNELS * sizeof(short)); /* Retain no pointer into adapter storage. */
    queue->frames += frames; /* Publish count only after the complete copy. */
    return FVM_OK; /* Acknowledge ownership transfer to the software FIFO. */
} /* End atomic software acceptance. */
/* Observe complete native FIFO frames without changing consumption or voice time. */
static FVM_Result queue_count(void *raw, size_t *out) /* The adapter validates output first. */
{ /* Begin native observation. */
    *out = ((ExampleQueue *)raw)->frames; /* Report the initialized complete-frame prefix. */
    return FVM_OK; /* No device or VM is consulted. */
} /* End native observation. */
/* Keep pause acknowledgement explicit even though this tutorial has no timed consumer. */
static FVM_Result queue_pause(void *raw, int paused) /* Adapter validates zero or one. */
{ /* Begin software control. */
    ((ExampleQueue *)raw)->paused = paused; /* Store the acknowledged policy. */
    return FVM_OK; /* No implicit FIFO clear occurs. */
} /* End software control. */
/* Clear is atomic because no file consumer has run while the adapter is active. */
static FVM_Result queue_clear(void *raw) /* No pending FILE side effect is rolled back. */
{ /* Begin native cancellation. */
    ((ExampleQueue *)raw)->frames = 0; /* Discard the initialized prefix by acknowledged policy. */
    return FVM_OK; /* The composition adapter may now discard its own pending retry. */
} /* End native cancellation. */
/* One exact SWF8 contains two event resources and a root PCM block, all at 44100 Hz. */
static const unsigned char movie[] = /* Keep input reproducible without an encoder. */
{ /* Begin literal SWF bytes. */
    0x46, 0x57, 0x53, 0x08, 0x6e, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x0c, /* Embedded bytes 0 through 11. */
    0x01, 0x00, 0x97, 0x03, 0x07, 0x00, 0x3e, 0x08, 0x00, 0x00, 0x00, 0x30, /* Embedded bytes 12 through 23. */
    0x75, 0x30, 0x75, 0x30, 0x75, 0x30, 0x75, 0x30, 0x75, 0x30, 0x75, 0x30, /* Embedded bytes 24 through 35. */
    0x75, 0x30, 0x75, 0x9f, 0x03, 0x08, 0x00, 0x3e, 0x0c, 0x00, 0x00, 0x00, /* Embedded bytes 36 through 47. */
    0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, /* Embedded bytes 48 through 59. */
    0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, 0xd0, 0x8a, /* Embedded bytes 60 through 71. */
    0xc3, 0x03, 0x07, 0x00, 0x00, 0xc3, 0x03, 0x08, 0x00, 0x00, 0x44, 0x0b, /* Embedded bytes 72 through 83. */
    0x0e, 0x3e, 0x08, 0x00, 0xd0, 0x04, 0x30, 0x75, 0xd0, 0x8a, 0x20, 0x4e, /* Embedded bytes 84 through 95. */
    0xe0, 0xb1, 0x10, 0x27, 0xf0, 0xd8, 0x30, 0x75, 0xd0, 0x8a, 0x40, 0x00, /* Embedded bytes 96 through 107. */
    0x00, 0x00, /* Embedded bytes 108 through 109. */
}; /* End the complete canonical input. */
/* Export a finite host-selected mixture; source ownership ends before any file is opened. */
int main(int argc, char **argv) /* Keep the reusable integration valid C89. */
{ /* Begin VM, independent native sources and file ownership. */
    FVM_Config config; /* Configure source loading before creating its runtime. */
    FVM_Runtime *runtime = 0; /* Own until all independent snapshots have been prepared. */
    FVM_Context *context = 0; /* Canonical SWF owner, never published to a native worker. */
    FVM_SWFStreamSoundSnapshot *source = 0; /* Own the independently bounded stream copy. */
    FVM_SWFStreamConverter converter; /* Borrow source until export completes. */
    FVM_SWFStreamConvertedChunk chunk; /* Receive per-block source/frame metadata. */
    FVM_SWFSoundMixerConfig geometry; /* Select explicit conversion of both event voices. */
    FVM_SWFSoundMixer *mixer = 0; /* Own event snapshots and their independent clocks. */
    short pcm[EXAMPLE_MIX_BLOCK * EXAMPLE_MIX_CHANNELS]; /* Own base, mixed output and retry bytes. */
    FVM_SWFSoundMixQueue *mix_queue = 0; /* Own retry storage before native publication. */
    FVM_AudioQueueBackend backend, adapted; /* Borrow real FIFO callbacks and wrapped composition. */
    FVM_SWFSoundMixQueueInfo pending; /* Observe retained retry ownership without a callback. */
    ExampleQueue queue; /* Keep the finite native FIFO alive through adapter cleanup. */
    int queue_mode = argc == 3 && !strcmp(argv[2], "--queue"); /* Explicit queue/retry tutorial. */
    FILE *output = 0; /* No file exists until successful source preparation and teardown. */
    size_t total = 0, count, values, mixed, event; /* Distinguish frames from channel values. */
    int eof = 0, status = 1; /* Stop stream reads at EOF; report failure until fclose succeeds. */
    FVM_Result result; /* Distinguish normal converter EOF from actual failure. */
    memset(&queue, 0, sizeof(queue)); /* Begin with no initialized FIFO frames. */
    if (argc != 2 && !queue_mode) /* Usage must not create an output file or start a VM. */
    { /* Describe the explicit file replacement. */
        printf("Usage: %s OUTPUT.pcm [--queue]\nReplaces OUTPUT.pcm with 16 native S16 stereo frames at 48000 Hz.\n", argv[0]); /* State output geometry. */
        return argc == 1 ? 0 : 1; /* Preserve no-side-effect help and reject extra arguments. */
    } /* Finish CLI validation. */
    if (!checked(fvm_config_init(&config), "config")) /* Initialize before any acquisition. */
        goto cleanup; /* No owner exists on this failure path. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime")) /* Acquire source runtime. */
        goto cleanup; /* Keep source ownership explicit. */
    if (!checked(fvm_context_create(runtime, &context), "context")) /* Acquire canonical owner. */
        goto cleanup; /* Release runtime after a context failure. */
    if (!checked(fvm_load_memory(context, movie, sizeof(movie)), "load")) /* Copy/validate SWF. */
        goto cleanup; /* No output file has been created. */
    result = fvm_swf_stream_sound_snapshot_open_pcm_bounded( /* Bound preparation peak. */
        context, 0, EXAMPLE_MIX_BUDGET, &source); /* Select the only root stream. */
    if (!checked(result, "stream")) /* Prepare PCM before native publication. */
        goto cleanup; /* Release partially acquired VM ownership. */
    geometry.struct_size = sizeof(geometry); /* Match the current native configuration ABI. */
    geometry.sample_rate = EXAMPLE_MIX_RATE; /* Share the explicit stream output clock. */
    geometry.channels = EXAMPLE_MIX_CHANNELS; /* Match interleaved caller storage. */
    geometry.voice_capacity = EXAMPLE_MIX_VOICES; /* Reserve exactly the two prepared events. */
    geometry.flags = FVM_SWF_SOUND_MIXER_CONVERT; /* Convert native source rates/channels explicitly. */
    if (!checked(fvm_swf_sound_mixer_create_configured(&geometry, &mixer), "mixer")) /* Reserve voices. */
        goto cleanup; /* No file side effect occurs on mixer failure. */
    for (event = 0; event < EXAMPLE_MIX_VOICES; ++event) /* This fixture schedules both starts at frame one. */
        if (!checked(fvm_swf_sound_mixer_submit(mixer, context, event), "event")) /* Owner snapshots event. */
            goto cleanup; /* Release any earlier successfully prepared voice. */
    result = fvm_swf_stream_converter_init( /* Borrow source without converter allocation. */
        &converter, source, EXAMPLE_MIX_RATE, EXAMPLE_MIX_CHANNELS); /* Match the mixer clock. */
    if (!checked(result, "converter")) goto cleanup; /* Preserve source/geometry errors. */
    if (queue_mode) /* Prepare adapter storage before source teardown and native-only production. */
    { /* Bind one exclusive software FIFO to the configured event mixer. */
        memset(&backend, 0, sizeof(backend)); /* Initialize the complete callback descriptor. */
        backend.struct_size = sizeof(backend); /* Match the current native callback ABI. */
        backend.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION; /* Select the supported table version. */
        backend.sample_rate = EXAMPLE_MIX_RATE; /* Match converter and event clocks. */
        backend.channels = EXAMPLE_MIX_CHANNELS; /* Match every caller/native frame. */
        backend.userdata = &queue; /* Borrow stable stack ownership until adapter close. */
        backend.write = queue_write; backend.queued = queue_count; /* Native copy/observe contract. */
        backend.pause = queue_pause; backend.clear = queue_clear; /* Explicit native controls. */
        result = fvm_swf_sound_mix_queue_create( /* Allocate original/mixed retry witnesses once. */
            mixer, &backend, EXAMPLE_MIX_BLOCK, &mix_queue, &adapted); /* Publish only complete state. */
        if (!checked(result, "mix queue")) goto cleanup; /* Release earlier source/mixer on failure. */
    } /* End owner-side preparation. */
    fvm_context_destroy(context); /* [GC:] End canonical ownership before the native loop. */
    context = 0; /* [GC:] Prevent cleanup from destroying the former context again. */
    fvm_runtime_destroy(runtime); /* Independent native sources no longer need their runtime. */
    runtime = 0; /* Prevent duplicate runtime destruction during fallback cleanup. */
    output = fopen(argv[1], "wb"); /* Replace only the explicit file after complete preparation. */
    if (!output) /* Surface the actual file-open error after native preparation. */
    { /* Preserve independent cleanup even when the selected directory is missing. */
        perror(argv[1]); /* Report the explicit path and native failure reason. */
        goto cleanup; /* No retry pretends to undo file side effects. */
    } /* Finish failed native acquisition. */
    while (total < EXAMPLE_MIX_TOTAL) /* Host chooses finite duration, including an event tail. */
    { /* Produce a new unmixed base exactly once for this output block. */
        count = EXAMPLE_MIX_TOTAL - total; /* Limit the final block to the finite export budget. */
        if (count > EXAMPLE_MIX_BLOCK) count = EXAMPLE_MIX_BLOCK; /* Preserve caller buffer bounds. */
        memset(pcm, 0, sizeof(pcm)); values = 0; /* Explicit zero base serves only a host-authorized tail. */
        if (!eof) /* Do not consult the converter after its normal exhaustion. */
        { /* Read at most one native stream chunk without crossing a source block. */
            chunk.struct_size = sizeof(chunk); /* Validate copied chunk metadata size. */
            result = fvm_swf_stream_converter_next( /* Convert only this bounded source slice. */
                &converter, count, pcm, /* Retain caller storage for later native output. */
                sizeof(pcm) / sizeof(short), &chunk, &values); /* Receive exact initialized values. */
            if (result == FVM_NOT_FOUND) eof = 1; /* A tail now uses the explicit zero base. */
            else if (!checked(result, "stream read")) goto cleanup; /* Never turn a decoding error into silence. */
            else if (!values) continue; /* Empty markers advance source metadata, not voice time. */
            else count = values / EXAMPLE_MIX_CHANNELS; /* Mix only initialized converted frames. */
        } /* Finish one source observation. */
        if (queue_mode) /* Queue accepts only the retained mixture, never a second mixer advance. */
        { /* Deliberately reject each new base before accepting its identical retry. */
            queue.reject = 1; /* Arm the native consumer before the first write attempt. */
            result = adapted.write(adapted.userdata, pcm, count); /* Mix once, then retain on reject. */
            if (result != FVM_NOT_FOUND) goto cleanup; /* Require actual controlled backpressure. */
            pending.struct_size = sizeof(pending); /* Match native retry observation geometry. */
            if (!checked(fvm_swf_sound_mix_queue_info(mix_queue, &pending), "pending")) goto cleanup; /* No device poll. */
            if (pending.pending_frames != count) goto cleanup; /* The adapter owns exactly this retry. */
            if (!checked(adapted.write(adapted.userdata, pcm, count), "retry")) goto cleanup; /* Reuse the original base unchanged. */
        } /* Retained mixed bytes have now moved to the native FIFO. */
        else /* The original direct-file demonstration still uses explicit in-place mixing. */
        { /* A file write failure remains terminal rather than a queue retry. */
            result = fvm_swf_sound_mixer_add( /* Sum base and all voices before clipping once. */
                mixer, count, pcm, sizeof(pcm) / sizeof(short), &mixed); /* Overwrite owned PCM. */
            if (!checked(result, "mix")) goto cleanup; /* Preserve ordinary capacity/owner errors. */
            if (mixed != count * EXAMPLE_MIX_CHANNELS) goto cleanup; /* Require complete mixing. */
            if (fwrite(pcm, sizeof(short), mixed, output) != mixed) goto cleanup; /* Terminal file failure. */
        } /* End one explicit output policy. */
        total += count; /* Advance the host output clock only after a complete write. */
    } /* End bounded serial export; a retrying backend must retain pcm instead of mixing again. */
    if (queue_mode) /* The file is a separate final consumer, outside queue retry semantics. */
    { /* Consume exactly the acknowledged native FIFO prefix once. */
        if (queue.frames != EXAMPLE_MIX_TOTAL) goto cleanup; /* Reject incomplete native export. */
        mixed = queue.frames * EXAMPLE_MIX_CHANNELS; /* Convert frames to scalar S16 values. */
        if (fwrite(queue.pcm, sizeof(short), mixed, output) != mixed) goto cleanup; /* Terminal I/O. */
        printf("Queue retained %lu rejected blocks.\n", (unsigned long)queue.rejected); /* Report real callbacks. */
    } /* No file consumer pretends to be an atomic queue backend. */
    status = fclose(output) == 0 ? 0 : 1; /* Retain the actual final flush/close result. */
    output = 0; /* A failed fclose still ends FILE ownership. */
    if (!status) printf("Mixed %lu frames after source VM teardown.\n", (unsigned long)total); /* Report the actual completed finite export. */
cleanup: /* Centralized cleanup also handles partial source preparation. */
    if (output) fclose(output); /* Close any file acquired before a later native error. */
    fvm_swf_sound_mix_queue_close(mix_queue); /* Quiesce wrapped callbacks before borrowed owners. */
    fvm_swf_sound_mixer_close(mixer); /* Release all remaining event voices; null is accepted. */
    fvm_swf_stream_sound_snapshot_close(source); /* Release independent source after converter use ends. */
    if (context) fvm_context_destroy(context); /* [GC:] Release source context if preparation failed. */
    if (runtime) fvm_runtime_destroy(runtime); /* Release runtime last on every preparation failure. */
    return status; /* Let C/C++ hosts observe ordinary success or failure. */
} /* End the complete C89 example. */
