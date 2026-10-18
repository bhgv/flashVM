/* C89 tutorial: replaceable PCM queue, retry ownership and dequeue clocks. */
#include "flashvm/swf_sound_output.h" /* Import the independent mixer/output/device ABI. */
#include <stdio.h> /* Print observable progress and ordinary error diagnostics. */
#include <string.h> /* Validate explicit modes before allocating resources. */
#define EXAMPLE_RATE 48000U /* Use a common device frame clock. */
#define EXAMPLE_CHANNELS 2U /* Interleave two signed-short channel values per frame. */
#define EXAMPLE_BLOCK 128U /* Prepare at most this many frames per pump call. */
#define EXAMPLE_TARGET 512U /* Bound the copied FIFO backlog separately from one block. */
#define EXAMPLE_DEVICE_BUFFER 256U /* Ask SDL for a power-of-two playback buffer. */
/* Keep a deterministic software FIFO model separate from the real SDL adapter. */
typedef struct DemoQueue /* This model counts silence frames; it is not a hardware backend. */
{ /* Begin host-owned callback state. */
    size_t frames; /* Count complete accepted frames that have not been consumed. */
    int paused; /* Preserve explicit pause state independently of the producer. */
    int reject; /* Reject the first complete write without consuming any of it. */
} DemoQueue; /* Retain this stack object until producer close. */
/* A production bridge must copy all interleaved samples before returning success. */
static FVM_Result write_pcm(void *raw, const short *pcm, size_t frames) /* Borrow one prepared block. */
{ /* Start one all-or-nothing write. */
    DemoQueue *queue = (DemoQueue *)raw; /* Recover exclusively owned model state. */
    (void)pcm; /* This silence-only model discards values; real bridges must copy them. */
    if (queue->reject) /* Demonstrate a recoverable backend failure. */
    { /* Do not advance FIFO state on a rejected write. */
        queue->reject = 0; /* Allow the next explicit retry to succeed. */
        return FVM_ERROR; /* Tell the producer to retain its exact pending block. */
    } /* Finish failure injection without partial acceptance. */
    queue->frames += frames; /* Accept every frame only on success. */
    return FVM_OK; /* Permit producer submitted-frame accounting to advance. */
} /* Finish the deterministic write adapter. */
/* Report remaining complete frames, including while paused. */
static FVM_Result queued_pcm(void *raw, size_t *frames) /* Publish one FIFO observation. */
{ /* Begin the nonblocking queue query. */
    *frames = ((DemoQueue *)raw)->frames; /* Copy the current frame count. */
    return FVM_OK; /* The model has no lost-device condition. */
} /* Finish queue observation. */
/* Pause controls consumption; it must not discard already accepted PCM. */
static FVM_Result pause_pcm(void *raw, int paused) /* Receive a validated Boolean state. */
{ /* Begin atomic state replacement. */
    ((DemoQueue *)raw)->paused = paused; /* Preserve all FIFO frames. */
    return FVM_OK; /* Publish success after applying the requested state. */
} /* Finish the pause adapter. */
/* Discard is a separate destructive operation used by stop/reset. */
static FVM_Result clear_pcm(void *raw) /* Borrow host queue ownership. */
{ /* Begin complete FIFO discard. */
    ((DemoQueue *)raw)->frames = 0; /* Remove every accepted but unconsumed frame. */
    return FVM_OK; /* Let the producer reset its clock epoch. */
} /* Finish queue discard. */
/* Report errors without assertions so the tutorial remains useful in Release builds. */
static int checked(FVM_Result result, const char *name) /* Borrow a stable diagnostic label. */
{ /* Begin status handling. */
    if (result == FVM_OK) return 1; /* Continue only after a successful operation. */
    fprintf(stderr, "%s: %d\n", name, (int)result); /* Preserve the exact API error code. */
    return 0; /* Select centralized cleanup. */
} /* Finish status handling. */
/* Show callback replacement without requiring a VM, an SWF or native audio permissions. */
int main(int argc, char **argv) /* Keep the host example portable C89. */
{ /* Begin separate mixer, producer and optional native-device ownership. */
    DemoQueue model; /* Keep the deterministic callback userdata alive until close. */
    FVM_AudioQueueBackend backend; /* Copy either custom or SDL callback operations. */
    FVM_SWFSoundMixerConfig geometry; /* Declare explicit output geometry independently of a source SWF. */
    FVM_SWFSoundMixer *mixer = 0; /* Own the fixed-capacity silence/event mixer. */
    FVM_SWFSoundOutput *output = 0; /* Own pending PCM but never the mixer or device. */
    FVM_SDLAudioQueue *device = 0; /* Own a native device only in explicit SDL mode. */
    FVM_SWFSoundOutputInfo info; /* Receive copied queue state and consumed-frame time. */
    size_t accepted = 0; /* Receive complete frames accepted by one pump. */
    int sdl = 0; /* Default to a deterministic model rather than opening hardware. */
    int status = 1; /* Report success only after cleanup-relevant operations succeed. */
    if (argc != 2 || (strcmp(argv[1], "--demo") && strcmp(argv[1], "--sdl"))) /* Validate before resource acquisition. */
    { /* Explain both explicitly selected paths. */
        fprintf(stderr, "usage: %s --demo|--sdl\n", argv[0]); /* SDL mode opens the default playback device. */
        return argc == 1 ? 0 : 2; /* Usage succeeds; malformed input fails. */
    } /* Finish argument validation. */
    sdl = strcmp(argv[1], "--sdl") == 0; /* Choose the backend before acquiring the mixer. */
    memset(&model, 0, sizeof(model)); /* Start with no accepted PCM. */
    model.paused = 1; model.reject = 1; /* Start paused and inject one retry in model mode. */
    memset(&backend, 0, sizeof(backend)); /* Avoid partially initialized native callbacks. */
    if (sdl) /* Acquire a real device only on explicit request. */
    { /* The disabled-SDK build returns UNSUPPORTED instead of a fake success. */
        if (!checked(fvm_audio_sdl_queue_open(EXAMPLE_RATE, EXAMPLE_CHANNELS, EXAMPLE_DEVICE_BUFFER, &device, &backend), "SDL open")) /* Copy exact geometry. */
            goto cleanup; /* No partial device is published on failure. */
    } /* Finish native backend acquisition. */
    else /* Install the independent deterministic host model. */
    { /* This table is borrowed through its copied userdata pointer. */
        backend.struct_size = sizeof(backend); /* Select the exact table layout. */
        backend.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION; /* Select the supported callback ABI. */
        backend.sample_rate = EXAMPLE_RATE; backend.channels = EXAMPLE_CHANNELS; /* Match mixer geometry. */
        backend.userdata = &model; /* Keep native state outside GC and the VM. */
        backend.write = write_pcm; backend.queued = queued_pcm; /* Separate all-or-nothing write and observation. */
        backend.pause = pause_pcm; backend.clear = clear_pcm; /* Separate pause and discard semantics. */
    } /* Finish replaceable backend setup. */
    geometry.struct_size = sizeof(geometry); /* Select the exact configured-mixer ABI. */
    geometry.sample_rate = EXAMPLE_RATE; geometry.channels = EXAMPLE_CHANNELS; /* Match the backend input. */
    geometry.voice_capacity = 1; geometry.flags = FVM_SWF_SOUND_MIXER_CONVERT; /* Permit a non-SWF output clock explicitly. */
    if (!checked(fvm_swf_sound_mixer_create_configured(&geometry, &mixer), "mixer")) /* Empty voices produce silence. */
        goto cleanup; /* Release an acquired SDL device if necessary. */
    if (!checked(fvm_swf_sound_output_create(mixer, &backend, EXAMPLE_BLOCK, EXAMPLE_TARGET, &output), "output")) /* Allocate pending storage and pause. */
        goto cleanup; /* Preserve caller ownership of mixer/device. */
    if (!sdl && fvm_swf_sound_output_pump(output, &accepted) != FVM_ERROR) /* Reject the model's first complete block. */
        goto cleanup; /* Unexpected behavior is an ordinary tutorial failure. */
    if (!checked(fvm_swf_sound_output_pump(output, &accepted), "pump/retry")) /* Retry identical PCM or perform the first native write. */
        goto cleanup; /* A rejected buffer remains owned by output until stop/close. */
    if (!checked(fvm_swf_sound_output_pause(output, 0), "resume")) /* Explicitly allow device consumption. */
        goto cleanup; /* Prepared data is never implicitly resumed. */
    if (!sdl && !model.paused) model.frames /= 2; /* Deterministically consume half a block in model mode. */
    info.struct_size = sizeof(info); /* Select the copied-info ABI before querying. */
    if (!checked(fvm_swf_sound_output_info(output, &info), "clock")) /* Observe queue depletion rather than wall time. */
        goto cleanup; /* Do not print unchecked output. */
    printf("accepted=%lu queued=%lu played=%.0f\n", (unsigned long)accepted, (unsigned long)info.queued_frames, info.played_frames); /* SDL counts are asynchronous. */
    if (!checked(fvm_swf_sound_output_stop(output), "stop")) /* Pause, discard, stop voices and reset the clock epoch. */
        goto cleanup; /* Close can still release ownership after a lost-device error. */
    status = 0; /* Every required operation completed successfully. */
cleanup: /* Preserve dependency order on success and every error path. */
    fvm_swf_sound_output_close(output); /* Free pending PCM before its borrowed dependencies. */
    fvm_swf_sound_mixer_close(mixer); /* Release software voices independently of the device. */
    if (!checked(fvm_audio_sdl_queue_close(device), "SDL close")) status = 1; /* Null succeeds and native close balances subsystem ownership. */
    return status; /* Never claim this short silence protocol is audible acceptance. */
} /* Finish the portable queue tutorial. */
