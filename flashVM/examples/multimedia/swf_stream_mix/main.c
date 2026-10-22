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
    FVM_SWFSoundVoice *prepared[EXAMPLE_MIX_VOICES] = {0}; /* Optional tracked voices stay caller-owned until adoption succeeds. */
    FVM_SWFSoundMixQueueIdentityInfo identity; /* Copy one sound-ID deadline before and after software drain. */
    FVM_SWFSoundMixer *mixer = 0; /* Own event snapshots and their independent clocks. */
    short pcm[EXAMPLE_MIX_BLOCK * EXAMPLE_MIX_CHANNELS]; /* Own base, mixed output and retry bytes. */
    FVM_SWFSoundMixQueue *mix_queue = 0; /* Own retry storage before native publication. */
    FVM_AudioQueueBackend backend, adapted; /* Borrow real FIFO callbacks and wrapped composition. */
    FVM_SWFSoundMixQueueInfo pending; /* Observe retained retry ownership without a callback. */
    FVM_SWFSoundMixQueueClock shared_clock; /* Copy shared counters rather than inferring time from stream EOF. */
    FVM_SWFSoundMixQueuePending ownership; /* Observe whether base or tail owns a rendered retry. */
    FVM_SWFSoundMixQueueStorageInfo storage; /* Copy fixed native allocation requests for the bounded mode. */
    size_t queue_budget = 0; /* Keep adapter storage separate from stream/voice/device budgets. */
    ExampleQueue queue; /* Keep the finite native FIFO alive through adapter cleanup. */
    int identity_mode = argc == 3 && !strcmp(argv[2], "--identities"); /* Keep sound IDs active through queued/pending PCM. */
    int clock_mode = (argc == 3 && !strcmp(argv[2], "--clock")) || identity_mode; /* Observe one shared accepted/dequeued epoch across base and tails. */
    int bounded_mode = (argc == 3 && !strcmp(argv[2], "--bounded-tails")) || clock_mode; /* Use an exact adapter-only allocation ceiling. */
    int tail_mode = (argc == 3 && !strcmp(argv[2], "--tails")) || bounded_mode; /* Select explicit pump_tail after actual converter EOF. */
    int queue_mode = (argc == 3 && !strcmp(argv[2], "--queue")) || tail_mode; /* Explicit queue/retry tutorial. */
    FILE *output = 0; /* No file exists until successful source preparation and teardown. */
    size_t total = 0, count, values, mixed, event, accepted = 0; /* Distinguish frames from channel values. */
    int eof = 0, status = 1; /* Stop stream reads at EOF; report failure until fclose succeeds. */
    FVM_Result result; /* Distinguish normal converter EOF from actual failure. */
    memset(&queue, 0, sizeof(queue)); /* Begin with no initialized FIFO frames. */
    if (argc != 2 && !queue_mode) /* Usage must not create an output file or start a VM. */
    { /* Describe the explicit file replacement. */
        printf("Usage: %s OUTPUT.pcm [--queue|--tails|--bounded-tails|--clock|--identities]\nReplaces OUTPUT.pcm with 16 native S16 stereo frames at 48000 Hz.\n", argv[0]); /* State output geometry. */
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
    { /* Tracked mode delays adoption until the empty queue clock and ID table exist. */
        result = identity_mode ? fvm_swf_sound_voice_open_bounded(context, event, EXAMPLE_MIX_VOICE_BYTES, &prepared[event]) /* Bound each temporary native voice snapshot independently. */
                               : fvm_swf_sound_mixer_submit(mixer, context, event); /* Existing modes retain direct owner-side admission. */
        if (!checked(result, "event preparation")) goto cleanup; /* Cleanup releases any caller-owned prepared handle. */
    } /* No source VM pointer is retained by native voices. */
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
        if (bounded_mode) /* Plan the exact fixed adapter requests before acquiring them. */
        { /* This does not include borrowed mixer, voices or software FIFO memory. */
            if (!checked(fvm_swf_sound_mix_queue_storage_required(EXAMPLE_MIX_CHANNELS, EXAMPLE_MIX_BLOCK, &queue_budget), "mix queue required")) goto cleanup; /* No allocation, callback or VM access. */
            result = fvm_swf_sound_mix_queue_create_bounded(mixer, &backend, EXAMPLE_MIX_BLOCK, queue_budget, &mix_queue, &adapted); /* Exact-fit native acquisition. */
        } /* An insufficient budget would fail before any adapter allocation. */
        else result = fvm_swf_sound_mix_queue_create(mixer, &backend, EXAMPLE_MIX_BLOCK, &mix_queue, &adapted); /* Existing modes retain unrestricted adapter acquisition. */
        if (!checked(result, "mix queue")) goto cleanup; /* Release earlier source/mixer on failure. */
        if (bounded_mode) /* Observe portable requested bytes rather than claiming process RSS. */
        { /* Require the documented storage descriptor size. */
            storage.struct_size = sizeof(storage); /* Preserve native ABI validation. */
            if (!checked(fvm_swf_sound_mix_queue_storage_info(mix_queue, &storage), "mix queue storage")) goto cleanup; /* No backend observation occurs. */
            if (storage.retained_bytes != queue_budget) goto cleanup; /* Predict and observe the same fixed requests. */
            printf("Mix queue buffers=%lu fixed=%lu retained=%lu\n", (unsigned long)storage.buffer_bytes, (unsigned long)storage.fixed_bytes, (unsigned long)storage.retained_bytes); /* Sizes are platform-dependent except for the fixed PCM arrays. */
        } /* Native hot-path buffers never grow. */
    } /* End owner-side preparation. */
    if (clock_mode && !checked(fvm_swf_sound_mix_queue_clock_begin(mix_queue), "shared clock begin")) goto cleanup; /* Explicitly require an empty real FIFO before counting accepted frames. */
    if (identity_mode) /* Preallocate the optional ID table before native output starts. */
    { /* Keep adapter table storage separate from mixer/voice/queue budgets. */
        if (!checked(fvm_swf_sound_mix_queue_storage_required_for_tracking(EXAMPLE_MIX_CHANNELS, EXAMPLE_MIX_BLOCK, 1, &queue_budget), "tracked storage")) goto cleanup; /* Predict owner, PCM and optional deadlines with checked native arithmetic. */
        if (!checked(fvm_swf_sound_mix_queue_set_identity_tracking(mix_queue, 1, queue_budget), "identity tracking")) goto cleanup; /* Require zero epoch and no voices before enabling. */
        for (event = 0; event < EXAMPLE_MIX_VOICES; ++event) /* Adopt every start through the tracked queue. */
            if (!checked(fvm_swf_sound_mix_queue_submit_prepared_bounded(mix_queue, &prepared[event], EXAMPLE_MIX_EVENT_BYTES), "tracked event")) goto cleanup; /* Success clears the caller handle and registers its exact end. */
    } /* All later ID controls also use this tracked owner. */
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
            result = tail_mode && eof ? fvm_swf_sound_mix_queue_pump_tail(mix_queue, count, EXAMPLE_MIX_TOTAL, &accepted) /* Render a zero-base event block only after converter exhaustion. */
                                     : adapted.write(adapted.userdata, pcm, count); /* Base input keeps its original-byte retry contract. */
            if (result != FVM_NOT_FOUND) goto cleanup; /* Require actual controlled backpressure. */
            pending.struct_size = sizeof(pending); /* Match native retry observation geometry. */
            if (!checked(fvm_swf_sound_mix_queue_info(mix_queue, &pending), "pending")) goto cleanup; /* No device poll. */
            ownership.struct_size = sizeof(ownership); /* Copy retry origin separately from the legacy Info ABI. */
            if (!checked(fvm_swf_sound_mix_queue_pending(mix_queue, &ownership), "pending origin")) goto cleanup; /* No queue callback or voice movement. */
            if (ownership.frames != count || ownership.origin != (tail_mode && eof ? FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL : FVM_SWF_SOUND_MIX_QUEUE_PENDING_BASE)) goto cleanup; /* Never let one producer steal the other's retry. */
            if (pending.pending_frames != count) goto cleanup; /* The adapter owns exactly this retry. */
            result = tail_mode && eof ? fvm_swf_sound_mix_queue_pump_tail(mix_queue, count, EXAMPLE_MIX_TOTAL, &accepted) /* Resend retained tail bytes without another voice advance. */
                                     : adapted.write(adapted.userdata, pcm, count); /* Base retry reuses the original bytes unchanged. */
            if (!checked(result, "retry")) goto cleanup; /* Never count a rejected block as accepted time. */
            if (tail_mode && eof && accepted != count) goto cleanup; /* Require the complete authorized tail block. */
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
        if (tail_mode && fvm_swf_sound_mix_queue_pump_tail(mix_queue, 1, EXAMPLE_MIX_TOTAL, &accepted) != FVM_NOT_FOUND) goto cleanup; /* Exhausted voices must not enqueue more silence. */
        if (tail_mode && accepted) goto cleanup; /* Exhaustion clears accepted without draining the FIFO. */
        if (queue.frames != EXAMPLE_MIX_TOTAL) goto cleanup; /* Reject incomplete native export. */
        if (clock_mode) /* All accepted base and tail blocks are still waiting in this software FIFO. */
        { /* Read a fresh validated observation without changing voice time. */
            shared_clock.struct_size = sizeof(shared_clock); /* Required public clock descriptor size. */
            if (!checked(fvm_swf_sound_mix_queue_clock_info(mix_queue, &shared_clock), "shared queued clock")) goto cleanup; /* No DAC position is claimed. */
            printf("Shared clock submitted=%.0f queued=%lu played=%.0f\n", shared_clock.submitted_frames, (unsigned long)shared_clock.queued_frames, shared_clock.played_frames); /* Expected 16, 16, zero before software consumption. */
        } /* Clock snapshots are owned values. */
        if (identity_mode) /* Mixer retirement alone cannot expire queued sound identity. */
        { /* ID8 is the longer event in the embedded SWF. */
            identity.struct_size = sizeof(identity); /* Require the exact copied observation ABI. */
            if (!checked(fvm_swf_sound_mix_queue_identity_info(mix_queue, EXAMPLE_MIX_LONG_ID, &identity), "queued identity")) goto cleanup; /* Observe real FIFO before publishing its deadline. */
            printf("Identity id=%u end=%.0f played=%.0f active=%d\n", identity.sound_id, identity.end_frame, identity.played_frames, identity.active); /* Expected ID8 end14 played0 active1. */
        } /* This reports dequeue registration, not physical audibility. */
        mixed = queue.frames * EXAMPLE_MIX_CHANNELS; /* Convert frames to scalar S16 values. */
        if (fwrite(queue.pcm, sizeof(short), mixed, output) != mixed) goto cleanup; /* Terminal I/O. */
        if (clock_mode) /* The successful file write is the explicit software FIFO consumer. */
        { /* Only acknowledge drain after every queued scalar was copied. */
            queue.frames = 0; /* Publish the actual software dequeue; this is not a device playhead. */
            if (!checked(fvm_swf_sound_mix_queue_clock_info(mix_queue, &shared_clock), "shared drained clock")) goto cleanup; /* Observe submitted-minus-backlog after consumption. */
            printf("Shared clock submitted=%.0f queued=%lu played=%.0f seconds=%.9f\n", shared_clock.submitted_frames, (unsigned long)shared_clock.queued_frames, shared_clock.played_frames, shared_clock.seconds); /* The count is 16 and seconds is 16/48000. */
            if (identity_mode) /* Observe expiry only after the software consumer acknowledged drain. */
            { /* Cancel the expired registration and disable at a drained safe point. */
                if (!checked(fvm_swf_sound_mix_queue_identity_info(mix_queue, EXAMPLE_MIX_LONG_ID, &identity), "drained identity")) goto cleanup; /* Retain the historical end in this copied snapshot. */
                printf("Identity id=%u end=%.0f played=%.0f active=%d\n", identity.sound_id, identity.end_frame, identity.played_frames, identity.active); /* Expected end14 played16 active0. */
                if (!checked(fvm_swf_sound_mix_queue_stop_all(mix_queue), "cancel identities")) goto cleanup; /* Cancel every future event registration without rewriting accepted PCM. */
                if (!checked(fvm_swf_sound_mix_queue_set_identity_tracking(mix_queue, 0, 0), "disable identities")) goto cleanup; /* Free optional deadlines only after voices/retries/FIFO have drained. */
            } /* Clock end is now permitted. */
            if (!checked(fvm_swf_sound_mix_queue_clock_end(mix_queue), "shared clock end")) goto cleanup; /* End only after retry ownership and FIFO have drained. */
        } /* The adapter remains alive until centralized cleanup. */
        printf("Queue retained %lu rejected blocks.\n", (unsigned long)queue.rejected); /* Report real callbacks. */
    } /* No file consumer pretends to be an atomic queue backend. */
    status = fclose(output) == 0 ? 0 : 1; /* Retain the actual final flush/close result. */
    output = 0; /* A failed fclose still ends FILE ownership. */
    if (!status) printf("Mixed %lu frames after source VM teardown.\n", (unsigned long)total); /* Report the actual completed finite export. */
cleanup: /* Centralized cleanup also handles partial source preparation. */
    for (event = 0; event < EXAMPLE_MIX_VOICES; ++event) fvm_swf_sound_voice_close(prepared[event]); /* Release only handles not consumed by successful tracked adoption. */
    if (output) fclose(output); /* Close any file acquired before a later native error. */
    fvm_swf_sound_mix_queue_close(mix_queue); /* Quiesce wrapped callbacks before borrowed owners. */
    fvm_swf_sound_mixer_close(mixer); /* Release all remaining event voices; null is accepted. */
    fvm_swf_stream_sound_snapshot_close(source); /* Release independent source after converter use ends. */
    if (context) fvm_context_destroy(context); /* [GC:] Release source context if preparation failed. */
    if (runtime) fvm_runtime_destroy(runtime); /* Release runtime last on every preparation failure. */
    return status; /* Let C/C++ hosts observe ordinary success or failure. */
} /* End the complete C89 example. */
