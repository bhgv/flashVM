/* C89 tutorial: queued NoMultiple lifetime, exact failed-write retry and explicit native control. */
#include "flashvm/swf_sound_output.h" /* Public native queue, identity, mixer and source ownership. */
#include "example_constants.h" /* Separate finite source/mixer/FIFO policies. */
#include "sample_movie.h" /* Independent original analytical PCM source. */
#include <stdio.h> /* Optional explicit PCM file output and visible errors. */
#include <string.h> /* Recognize the usage-only option before acquiring owners. */
/* A finite caller-owned FIFO model, not an OS device or a hardware playback clock. */
typedef struct ExampleHost /* Backend userdata lives until the output is closed. */
{ /* Begin explicit native queue state. */
    short pcm[EXAMPLE_ID_VALUES]; /* Concatenate only accepted PCM, even across explicit clear. */
    size_t values; /* Initialized scalar S16 evidence values. */
    size_t queued; /* Explicit complete-frame FIFO backlog. */
    int paused; /* Last acknowledged host pause state. */
    int reject; /* Deliberately reject exactly one complete write without accepting samples. */
} ExampleHost; /* End the independent backend model. */
/* Report the original API result while leaving partial ownership with main. */
static int checked(FVM_Result result, const char *operation) /* No assertions in the tutorial. */
{ /* Begin status reporting. */
    if (result == FVM_OK) return 1; /* Continue after explicit success only. */
    fprintf(stderr, "%s: %d\n", operation, (int)result); /* Preserve actual failure status. */
    return 0; /* Centralized cleanup remains responsible for every owner. */
} /* End status reporting. */
/* Accept all PCM by copying it, or accept none on any failure. */
static FVM_Result host_write(void *raw, const short *pcm, size_t frames) /* Native-only callback. */
{ /* Begin atomic model acceptance. */
    ExampleHost *host = (ExampleHost *)raw; /* Borrow live caller state through return. */
    size_t i; /* Iterate initialized scalar channel values. */
    if (host->reject) { host->reject = 0; return FVM_ERROR; } /* Consume only the failure injection flag. */
    if ((frames && !pcm) || frames > (EXAMPLE_ID_VALUES - host->values) / EXAMPLE_ID_CHANNELS) /* Bound copy. */
        return FVM_OUT_OF_MEMORY; /* Leave PCM evidence and queued frames unchanged. */
    for (i = 0; i < frames * EXAMPLE_ID_CHANNELS; ++i) /* Copy before acknowledging any acceptance. */
        host->pcm[host->values + i] = pcm[i]; /* Backend never retains the producer's pointer. */
    host->values += frames * EXAMPLE_ID_CHANNELS; /* Commit the complete evidence slice. */
    host->queued += frames; /* Commit exactly the same complete-frame backlog. */
    return FVM_OK; /* All input PCM is now caller-owned backend data. */
} /* End atomic model acceptance. */
/* Observe the exclusive FIFO without guessing device latency or running the source VM. */
static FVM_Result host_queued(void *raw, size_t *out) /* Required native observation callback. */
{ /* Begin copied queue observation. */
    *out = ((ExampleHost *)raw)->queued; /* Publish the explicitly modeled complete-frame count. */
    return FVM_OK; /* The model cannot fail this observation. */
} /* End queue observation. */
/* Pause changes only host consumption policy; it never clears or rewinds PCM. */
static FVM_Result host_pause(void *raw, int paused) /* Output validates the Boolean argument. */
{ /* Begin explicit pause acknowledgement. */
    ((ExampleHost *)raw)->paused = paused; /* Preserve every queued frame. */
    return FVM_OK; /* Acknowledge before output copies pause state. */
} /* End pause acknowledgement. */
/* Clear discards backlog while preserving the independent accepted-PCM evidence. */
static FVM_Result host_clear(void *raw) /* Called only while the host retains userdata. */
{ /* Begin acknowledged FIFO discard. */
    ((ExampleHost *)raw)->queued = 0; /* No independent producer or device may edit this queue. */
    return FVM_OK; /* Output may now reset its epoch and identity registrations. */
} /* End acknowledged discard. */
/* Prepare once, destroy the VM, then exercise all controls through one serialized output owner. */
int main(int argc, char **argv) /* Optional explicit destination receives accepted native S16. */
{ /* Begin the bounded native demonstration. */
    FVM_Config config; /* Initialized source runtime configuration. */
    FVM_Runtime *runtime = 0; /* Source runtime ends before native output starts. */
    FVM_Context *context = 0; /* Source owner needed only during preparation. */
    FVM_SWFSoundVoice *first = 0, *duplicate = 0, *replay = 0; /* Independent untransferred owners. */
    FVM_SWFSoundMixer *mixer = 0; /* Borrowed exclusively by output until it closes. */
    FVM_SWFSoundOutput *output = 0; /* Own retry/history/ID storage, borrow mixer and backend. */
    FVM_SWFSoundMixerConfig geometry; /* Explicit native conversion and single-slot geometry. */
    FVM_AudioQueueBackend backend; /* Copied native callback table borrowing host. */
    FVM_SWFSoundMixerInfo mixer_info; /* Observe software retirement independently of queued activity. */
    FVM_SWFSoundOutputStorageInfo storage; /* Observe separate fixed/output/identity charges. */
    FVM_SWFSoundOutputInfo info; /* Copy accepted versus pending ownership after write failure. */
    ExampleHost host; /* Stack owner outlives all borrowed backend calls. */
    FILE *file = 0; /* Optional explicitly named terminal file consumer. */
    size_t accepted = 0; /* One pump's acknowledged frame count. */
    double duration = 0; /* Exact rational output duration, independent of the whole block. */
    int playing = 0, status = 1; /* Copied ID state and pessimistic final result. */
    FVM_Result result; /* Preserve the deliberately injected write error. */
    if (argc > 2 || (argc == 2 && strcmp(argv[1], "--help") == 0)) /* Validate before any acquisition. */
    { /* Explain the optional side effect. */
        puts("Usage: swf_queued_identity [OUTPUT.pcm]"); /* No implicit filesystem destination. */
        return argc > 2 ? 1 : 0; /* Reject invalid CLI; usage-only succeeds. */
    } /* End early usage handling. */
    memset(&host, 0, sizeof(host)); host.paused = 1; /* Initialize complete empty native model state. */
    if (!checked(fvm_config_init(&config), "config")) goto cleanup; /* Initialize all runtime fields. */
    config.memory_limit_bytes = EXAMPLE_ID_RUNTIME_BYTES; /* Bound source runtime payloads separately. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime")) goto cleanup; /* Acquire source runtime. */
    if (!checked(fvm_context_create(runtime, &context), "context")) goto cleanup; /* Acquire source owner. */
    if (!checked(fvm_load_memory(context, example_movie, sizeof(example_movie)), "load")) goto cleanup; /* Copy source. */
    if (!checked(fvm_swf_sound_voice_open_bounded(context, 0, EXAMPLE_ID_MIXER_BYTES, &first), "first")) goto cleanup; /* Decode native copy. */
    if (!checked(fvm_swf_sound_voice_open_bounded(context, 1, EXAMPLE_ID_MIXER_BYTES, &duplicate), "duplicate")) goto cleanup; /* NoMultiple owner. */
    if (!checked(fvm_swf_sound_voice_clone(duplicate, EXAMPLE_ID_MIXER_BYTES, &replay), "replay")) goto cleanup; /* No repeated decode. */
    fvm_context_destroy(context); context = 0; /* Every prepared owner now survives independently. */
    fvm_runtime_destroy(runtime); runtime = 0; /* Complete source teardown before output acquisition. */
    if (!checked(fvm_swf_sound_voice_output_frames(first, EXAMPLE_ID_RATE, &duration), "duration")) goto cleanup; /* No PCM read. */
    if (duration != EXAMPLE_ID_END) goto cleanup; /* Compare with the independent seven-frame ratio. */
    geometry.struct_size = sizeof(geometry); geometry.sample_rate = EXAMPLE_ID_RATE; /* Initialize exact mixer ABI. */
    geometry.channels = EXAMPLE_ID_CHANNELS; geometry.voice_capacity = EXAMPLE_ID_VOICES; /* Bound conversion/slots. */
    geometry.flags = FVM_SWF_SOUND_MIXER_CONVERT; /* Explicit 44.1 kHz mono to 48 kHz stereo. */
    if (!checked(fvm_swf_sound_mixer_create_bounded(&geometry, EXAMPLE_ID_MIXER_BYTES, &mixer), "mixer")) goto cleanup; /* Fixed native owner. */
    backend.struct_size = sizeof(backend); backend.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION; /* Exact callback ABI. */
    backend.sample_rate = EXAMPLE_ID_RATE; backend.channels = EXAMPLE_ID_CHANNELS; /* Match mixer output. */
    backend.userdata = &host; backend.write = host_write; backend.queued = host_queued; /* Borrow caller FIFO. */
    backend.pause = host_pause; backend.clear = host_clear; /* Explicit pause/discard acknowledgement. */
    if (!checked(fvm_swf_sound_output_create(mixer, &backend, EXAMPLE_ID_BLOCK, EXAMPLE_ID_TARGET, &output), "output")) goto cleanup; /* Empty queue. */
    if (!checked(fvm_swf_sound_output_set_identity_tracking(output, 1), "tracking")) goto cleanup; /* Allocate fixed ID table once. */
    storage.struct_size = sizeof(storage); /* Exact output storage observation ABI. */
    if (!checked(fvm_swf_sound_output_storage_info(output, &storage), "output bytes")) goto cleanup; /* Excludes borrowed mixer/host. */
    if (!checked(fvm_swf_sound_output_submit_prepared_bounded(output, &first, EXAMPLE_ID_MIXER_BYTES), "adopt first")) goto cleanup; /* Consume handle. */
    if (!checked(fvm_swf_sound_output_pump(output, &accepted), "first pump")) goto cleanup; /* Accept the whole sixteen-frame block. */
    mixer_info.struct_size = sizeof(mixer_info); /* Observe the native mixer, never read it directly. */
    if (!checked(fvm_swf_sound_mixer_info(mixer, &mixer_info), "retirement")) goto cleanup; /* Nonmutating borrowed observation. */
    if (mixer_info.active_voices || accepted != EXAMPLE_ID_BLOCK) goto cleanup; /* Voice ended before its queued PCM. */
    if (!checked(fvm_swf_sound_output_sound_active(output, EXAMPLE_ID_SOUND, &playing), "queued ID")) goto cleanup; /* Real model FIFO clock. */
    if (!playing) goto cleanup; /* Paused unplayed PCM still reserves the dictionary identity. */
    if (!checked(fvm_swf_sound_output_submit_prepared_bounded(output, &duplicate, 0), "suppress duplicate")) goto cleanup; /* Consume without new storage. */
    if (duplicate) goto cleanup; /* Success acknowledges ownership even when NoMultiple is suppressed. */
    if (!checked(fvm_swf_sound_output_pause(output, 0), "resume")) goto cleanup; /* Permit explicit modeled consumption. */
    if (!host.paused) host.queued -= EXAMPLE_ID_END - 1; /* Consume seven frames, retaining the last voiced frame. */
    if (!checked(fvm_swf_sound_output_sound_active(output, EXAMPLE_ID_SOUND, &playing), "before endpoint")) goto cleanup; /* Still active. */
    if (!playing) goto cleanup; /* The exact eight-frame endpoint has not been reached. */
    --host.queued; /* Explicitly consume the final voiced frame; trailing silence remains queued. */
    if (!checked(fvm_swf_sound_output_sound_active(output, EXAMPLE_ID_SOUND, &playing), "endpoint")) goto cleanup; /* Reaches exact exclusive end. */
    if (playing) goto cleanup; /* Do not hold ID until the sixteen-frame block ends. */
    if (!checked(fvm_swf_sound_output_submit_prepared_bounded(output, &replay, EXAMPLE_ID_MIXER_BYTES), "restart")) goto cleanup; /* New NoMultiple now allowed. */
    host.reject = 1; /* Deliberately reject one complete backend write, accepting no bytes. */
    result = fvm_swf_sound_output_pump(output, &accepted); /* Prepare voice once and retain the exact rejected PCM. */
    if (result != FVM_ERROR || accepted) goto cleanup; /* This explicit injected failure is expected. */
    info.struct_size = sizeof(info); /* Copy pending versus accepted ownership. */
    if (!checked(fvm_swf_sound_output_info(output, &info), "pending")) goto cleanup; /* No decoder or VM. */
    if (info.pending_frames != EXAMPLE_ID_BLOCK) goto cleanup; /* Retry must own the complete prepared block. */
    if (!checked(fvm_swf_sound_output_pump(output, &accepted), "exact retry")) goto cleanup; /* Accept retained bytes without remixing. */
    if (accepted != EXAMPLE_ID_BLOCK || host.values != EXAMPLE_ID_EXPECTED_VALUES) goto cleanup; /* Exactly two accepted blocks. */
    if (memcmp(host.pcm, host.pcm + EXAMPLE_ID_BLOCK * EXAMPLE_ID_CHANNELS, /* Compare repeated accepted evidence. */
        EXAMPLE_ID_BLOCK * EXAMPLE_ID_CHANNELS * sizeof(short)) != 0) goto cleanup; /* Rejection did not advance twice. */
    if (!checked(fvm_swf_sound_output_stop_id(output, EXAMPLE_ID_SOUND), "stop ID")) goto cleanup; /* Release ID, leave accepted PCM intact. */
    if (!checked(fvm_swf_sound_output_sound_active(output, EXAMPLE_ID_SOUND, &playing), "stopped ID")) goto cleanup; /* No source access. */
    if (playing) goto cleanup; /* Explicit control releases NoMultiple registration. */
    if (!checked(fvm_swf_sound_output_stop(output), "stop output")) goto cleanup; /* Acknowledge clear before freeing output. */
    if (argc == 2) /* File output is optional and begins only after successful native demonstration. */
    { /* Begin terminal native-endian PCM export. */
        file = fopen(argv[1], "wb"); /* Replace only the explicitly requested destination. */
        if (!file || fwrite(host.pcm, sizeof(short), host.values, file) != host.values) goto cleanup; /* No file retry guarantee. */
        if (fclose(file) != 0) { file = 0; goto cleanup; } /* Keep flush failures visible. */
        file = 0; /* End file ownership exactly once. */
    } /* End optional file output. */
    printf("queued_duplicate=consumed retired=1 endpoint=%.0f accepted_frames=%lu identity_bytes=%lu after_vm_teardown=1\n", /* Finite model evidence. */
        duration, (unsigned long)(host.values / EXAMPLE_ID_CHANNELS), (unsigned long)storage.identity_bytes); /* Not DAC timing. */
    status = 0; /* Every explicit ownership and sample check passed. */
cleanup: /* Keep borrowed mixer/backend alive until output callbacks are quiescent. */
    if (file) fclose(file); /* A late file error may leave partial output. */
    if (output) fvm_swf_sound_output_stop(output); /* Best-effort native queue cancellation on failure. */
    fvm_swf_sound_output_close(output); /* Release retry/table storage before borrowed owners. */
    fvm_swf_sound_mixer_close(mixer); /* Release any adopted voice after output stops. */
    fvm_swf_sound_voice_close(first); fvm_swf_sound_voice_close(duplicate); fvm_swf_sound_voice_close(replay); /* Untransferred owners only. */
    if (context) fvm_context_destroy(context); /* Release partial source preparation. */
    if (runtime) fvm_runtime_destroy(runtime); /* Source runtime ends last when still present. */
    return status; /* Nonzero reports every unexpected API/queue/file result. */
} /* End serialized native identity tutorial. */
