/* C89 tutorial: explicit event controls, incremental mixing and native snapshot ownership. */
#include "flashvm/movieclip.h" /* Import real frame-entry delivery and callback registration. */
#include "flashvm/standard.h" /* Install MovieClip standard methods before timeline execution. */
#include "flashvm/gc.h" /* Pin the imported root during host ownership. */
#include "flashvm/swf_sound_mixer.h" /* Import the mixer and its underlying sound API. */
#include "example_constants.h" /* Give fixture indices and host geometry separate names. */
#include <stdio.h> /* Report failures and optionally write an explicitly named PCM file. */
#include <string.h> /* Check arguments before acquiring any resources. */
/* Keep a tiny canonical SWF fixture readable by describing each wire record. */
static const unsigned char swf[] = /* Encode two resources with independent IDs. */
{ /* Begin one-frame SWF8 with exact declared length 60 bytes. */
    'F', 'W', 'S', 8, 60, 0, 0, 0, 8, 0, 0, 12, 1, 0, /* Signature, length, RECT, FPS and frame count. */
    0x89, 3, 7, 0, 12, 2, 0, 0, 0, 0, 255, /* DefineSound 7: U8 mono, 44100 Hz, two samples. */
    0x89, 3, 8, 0, 12, 2, 0, 0, 0, 0, 255, /* DefineSound 8: identical PCM, different identity. */
    0xc3, 3, 7, 0, 0, /* Event 0 starts one independent resource-7 voice. */
    0xc3, 3, 7, 0, 16, /* Event 1 requests SyncNoMultiple for resource 7. */
    0xc3, 3, 7, 0, 32, /* Event 2 requests stopping all resource-7 voices. */
    0xc3, 3, 8, 0, 0, /* Event 3 starts resource 8. */
    0x40, 0, 0, 0 /* Complete ShowFrame and End tags. */
}; /* Finish the immutable raw input fixture. */
/* Handle actual API failures with ordinary control flow, without tutorial assertions. */
static int checked(FVM_Result result, const char *operation) /* Borrow the diagnostic label. */
{ /* Begin synchronous status handling. */
    if (result == FVM_OK) /* Continue only after a completed operation. */
        return 1; /* Tell the caller its next operation is safe. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Retain the exact VM status. */
    return 0; /* Use main's centralized ownership cleanup. */
} /* Finish status handling. */
/* Connect real root StartSound delivery to the independent software mixer. */
static FVM_Result timeline_sound(FVM_Context *context, FVM_Object *clip, size_t event, void *raw) /* Borrow host state. */
{ /* Enter synchronously at actual frame entry, before DoAction execution. */
    (void)clip; /* This root-only example needs the event ordinal rather than instance state. */
    return fvm_swf_sound_mixer_submit((FVM_SWFSoundMixer *)raw, context, event); /* Preserve preparation errors. */
} /* Finish the replaceable frame-to-output bridge. */
/* Advance the software mixer explicitly; no hidden timer or audio device is opened. */
int main(int argc, char **argv) /* Keep the runnable example C89-compatible. */
{ /* Begin independent VM, software mixer and optional output ownership. */
    FVM_Config config; /* Initialize this before constructing the runtime. */
    FVM_Runtime *runtime = 0; /* Own the runtime until explicit destruction. */
    FVM_Context *context = 0; /* Own the loaded source until snapshots have been submitted. */
    FVM_SWFSoundMixer *mixer = 0; /* Own software voices independently of the source context. */
    FVM_SWFSoundMixerConfig geometry; /* Copy an explicit output conversion policy without changing the old info ABI. */
    FVM_SWFSoundMixerInfo info; /* Receive copied geometry/activity, never a borrowed pointer. */
    FVM_Object *clip = 0; /* Borrow the managed root while its explicit pin is held. */
    int pinned = 0; /* Remember whether root ownership was successfully acquired. */
    int converted = 0; /* Enable rate/channel conversion only when explicitly requested. */
    unsigned int channels = EXAMPLE_CHANNELS; /* Use the actual output geometry for writes. */
    int timeline = 0; /* Choose actual frame delivery only in the explicit timeline mode. */
    FILE *output = 0; /* Open a file only when the user supplied its path. */
    short first[EXAMPLE_OUTPUT_CHANNELS]; /* Store the first saturated frame. */
    short tail[EXAMPLE_TAIL_FRAMES * EXAMPLE_OUTPUT_CHANNELS]; /* Keep remaining output and padded silence. */
    size_t written = 0; /* Receive counts in channel values rather than sample frames. */
    int status = 1; /* Assume failure until every operation and optional write succeeds. */
    if ((argc != 2 && argc != 3) || /* Accept one mode and at most one explicit output path. */
        (strcmp(argv[1], "--demo") != 0 && /* Recognize manual source-rate mixing. */
         strcmp(argv[1], "--timeline-demo") != 0 && /* Recognize actual root timeline delivery. */
         strcmp(argv[1], "--convert-demo") != 0)) /* Reject unknown modes before allocation/I/O. */
    { /* Explain the only accepted invocation. */
        fprintf(stderr, "usage: %s --demo|--timeline-demo|--convert-demo [output-native-s16.pcm]\n", argv[0]); /* Name the explicit output. */
        return argc == 1 ? 0 : 2; /* Usage alone succeeds; malformed arguments fail. */
    } /* Finish argument validation. */
    converted = strcmp(argv[1], "--convert-demo") == 0; /* Opt in before creating any resources. */
    timeline = converted || strcmp(argv[1], "--timeline-demo") == 0; /* Select scheduling before acquiring VM resources. */
    if (!checked(fvm_config_init(&config), "config")) /* Obtain the supported default configuration. */
        goto cleanup; /* Do not construct dependent objects after failure. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime")) /* Acquire the native runtime. */
        goto cleanup; /* Preserve null ownership on failure. */
    if (!checked(fvm_context_create(runtime, &context), "context")) /* Acquire the source VM. */
        goto cleanup; /* Release only objects actually acquired. */
    if (!checked(fvm_load_memory(context, swf, sizeof(swf)), "load")) /* Load canonical dictionary/events. */
        goto cleanup; /* Reject malformed or unsupported source data. */
    if (converted) /* The explicit third mode targets a common stereo device format. */
    { /* Configuration is copied; the stack object does not need to outlive the mixer. */
        geometry.struct_size = sizeof(geometry); /* Select the exact configured-creation ABI. */
        geometry.sample_rate = EXAMPLE_OUTPUT_RATE; /* Request 48000 output frames per second. */
        geometry.channels = EXAMPLE_OUTPUT_CHANNELS; /* Duplicate mono into stereo in this fixture. */
        geometry.voice_capacity = EXAMPLE_VOICES; /* Keep the same bounded polyphony policy. */
        geometry.flags = FVM_SWF_SOUND_MIXER_CONVERT; /* Explicitly allow source/output mismatch. */
        if (!checked(fvm_swf_sound_mixer_create_configured(&geometry, &mixer), "converted mixer")) /* Allocate fixed buffers. */
            goto cleanup; /* No partial mixer is published on failure. */
        if (!checked(fvm_swf_sound_mixer_get_config(mixer, &geometry), "output geometry")) /* Copy the actual policy. */
            goto cleanup; /* Never consume unchecked output geometry. */
        channels = geometry.channels; /* Write exactly the returned interleaved channel count. */
    } /* Interpolation and channel conversion occur during subsequent reads. */
    else /* Preserve the original strict mono SWF-rate mode. */
    { /* Existing callers never opt in to conversion accidentally. */
        if (!checked(fvm_swf_sound_mixer_create(EXAMPLE_RATE, EXAMPLE_CHANNELS, EXAMPLE_VOICES, &mixer), "mixer")) /* Allocate strict slots. */
            goto cleanup; /* Retain source resources for centralized cleanup. */
    } /* Finish fixed-policy mixer creation. */
    if (timeline) /* The alternate mode delivers all four controls from the actual first frame. */
    { /* Keep the imported root alive until after its synchronous callbacks have finished. */
        if (!checked(fvm_standard_install(context), "standard objects")) /* Install actual MovieClip methods. */
            goto cleanup; /* Retain centralized cleanup on failure. */
        if (!checked(fvm_movieclip_load_swf(context, swf, sizeof(swf), "root", &clip), "scene")) /* Match loaded source. */
            goto cleanup; /* No partial clip is published on import failure. */
        if (!checked(fvm_gc_pin(context, clip), "root pin")) /* [GC:] Own the imported frame/source graph. */
            goto cleanup; /* Never use a root after failed pinning. */
        pinned = 1; /* [GC:] Record ownership for both success and failure cleanup. */
        if (!checked(fvm_movieclip_set_sound_callback(context, timeline_sound, mixer), "sound callback")) /* Borrow mixer. */
            goto cleanup; /* Callback userdata is never owned by the VM. */
        if (!checked(fvm_movieclip_advance(context, clip, 1), "timeline entry")) /* Run start/no-multiple/stop/other. */
            goto cleanup; /* Propagate the actual event or script failure. */
    } /* Only ID 8 remains active after the first frame's ID-7 stop. */
    else /* Preserve the original explicit-control overlap demonstration. */
    { /* Manual controls remain useful for custom event schedulers. */
    if (!checked(fvm_swf_sound_mixer_submit(mixer, context, EXAMPLE_START), "first voice")) /* Snapshot ID 7. */
        goto cleanup; /* No partial voice is published on error. */
    if (!checked(fvm_swf_sound_mixer_submit(mixer, context, EXAMPLE_START), "overlapping voice")) /* Permit ordinary duplication. */
        goto cleanup; /* Existing voices remain valid on preparation failure. */
    if (!checked(fvm_swf_sound_mixer_submit(mixer, context, EXAMPLE_NO_MULTIPLE), "no-multiple")) /* Keep two ID-7 copies. */
        goto cleanup; /* No-multiple never steals another voice. */
    if (!checked(fvm_swf_sound_mixer_submit(mixer, context, EXAMPLE_OTHER), "other ID")) /* Fill the third slot with ID 8. */
        goto cleanup; /* Capacity errors remain visible. */
    } /* Both modes now provide an active voice for the same explicit PCM reader. */
    if (!checked(fvm_swf_sound_mixer_read(mixer, 1, first, EXAMPLE_OUTPUT_CHANNELS, &written), "first frame")) /* Mix the currently active voices once. */
        goto cleanup; /* Both modes produce a first signed-16 sample of -32768. */
    if (!timeline && !checked(fvm_swf_sound_mixer_submit(mixer, context, EXAMPLE_STOP), "stop ID 7")) /* Remove both ID-7 copies. */
        goto cleanup; /* ID 8 continues from its own current position. */
    if (pinned) /* [GC:] Release the host root before destroying its source context. */
        fvm_gc_unpin(context, clip); /* [GC:] No managed pointer is used by later PCM reads. */
    pinned = 0; /* [GC:] Prevent duplicate unpin during ordinary cleanup. */
    fvm_context_destroy(context); /* Demonstrate independence from canonical source bytes and GC. */
    context = 0; /* Prevent a second destruction on cleanup. */
    fvm_runtime_destroy(runtime); /* The mixer retains no runtime pointer. */
    runtime = 0; /* Record that native runtime ownership ended. */
    if (!checked(fvm_swf_sound_mixer_read(mixer, EXAMPLE_TAIL_FRAMES, tail, EXAMPLE_TAIL_FRAMES * EXAMPLE_OUTPUT_CHANNELS, &written), "tail")) /* Continue after VM teardown. */
        goto cleanup; /* Produce the remaining converted/strict samples and silence. */
    info.struct_size = sizeof(info); /* Declare the exact ABI structure size. */
    if (!checked(fvm_swf_sound_mixer_info(mixer, &info), "activity")) /* Observe exhausted voices. */
        goto cleanup; /* Do not read uninitialized copied fields. */
    if (argc == 3) /* File creation is authorized only by an explicit output argument. */
    { /* Write actual native signed-short PCM through the C runtime. */
        output = fopen(argv[2], "wb"); /* Create or replace the named output. */
        if (!output || fwrite(first, sizeof(short), channels, output) != channels) /* Check first-buffer I/O. */
            goto cleanup; /* Leave any FILE owned until centralized close. */
        if (fwrite(tail, sizeof(short), written, output) != written) /* Check the complete tail write. */
            goto cleanup; /* Preserve actual output failure. */
    } /* Finish optional output; no device permission is needed. */
    printf("first=%d tail=%d active=%u\n", (int)first[0], (int)tail[0], info.active_voices); /* Show actual computed values. */
    if (!checked(fvm_swf_sound_mixer_stop_all(mixer), "stop all")) /* Safe even after every voice has ended. */
        goto cleanup; /* Stop software state separately from any device queue. */
    status = 0; /* Mark the complete demonstration successful. */
cleanup: /* Release resources acquired before any failure. */
    if (output && fclose(output) != 0) /* Flush and close the real output exactly once. */
        status = 1; /* A failed final flush must not become success. */
    if (status && argc == 3) /* Supply a useful diagnostic for output or API failures. */
        fprintf(stderr, "demo/output failed: %s\n", argv[2]); /* Do not silently hide a partial file. */
    fvm_swf_sound_mixer_close(mixer); /* Free native voices even if the source VM was destroyed earlier. */
    if (pinned) /* [GC:] Cover errors after import/pin and before normal VM teardown. */
        fvm_gc_unpin(context, clip); /* [GC:] End only successfully acquired root ownership. */
    if (context) /* Handle source initialization failure safely. */
        fvm_context_destroy(context); /* Release any source state still owned here. */
    if (runtime) /* The runtime must outlive an existing source context. */
        fvm_runtime_destroy(runtime); /* Finish native VM ownership. */
    return status; /* Report actual API/I/O success to the caller. */
} /* End the allocation-free output-path demonstration. */
