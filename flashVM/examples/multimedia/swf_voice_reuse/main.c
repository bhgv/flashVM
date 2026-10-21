/* C89 tutorial: decode once, destroy the VM, clone/restart and export two finite native replays. */
#include "flashvm/swf_sound_mixer.h" /* Independent voice, budget and mixer ownership APIs. */
#include "example_constants.h" /* Explicit input, storage and finite-output policy. */
#include <stdio.h> /* Open only caller-selected binary files. */
#include <stdlib.h> /* Own the bounded temporary input copy. */
/* Keep actual errors visible while preserving one centralized cleanup path. */
static int checked(FVM_Result result, const char *operation) /* Translate one API result. */
{ /* Begin status reporting. */
    if (result == FVM_OK) return 1; /* Continue only after acknowledged success. */
    fprintf(stderr, "%s: %d\n", operation, (int)result); /* Preserve the original result. */
    return 0; /* Let main release its partial owners. */
} /* End status reporting. */
/* All source VM work finishes before native clones, mixer commands or output file acquisition. */
int main(int argc, char **argv) /* INPUT.swf OUTPUT.pcm selects the first root StartSound. */
{ /* Begin caller-owned replay lifetime. */
    FVM_Config config; /* Initialized bounded source runtime policy. */
    FVM_Runtime *runtime = 0; /* Destroy after its context, before native replay. */
    FVM_Context *context = 0; /* Needed only to prepare the template. */
    FVM_SWFSoundVoice *source = 0; /* Immutable template retained independently of the VM. */
    FVM_SWFSoundVoice *copy = 0; /* Untransferred replay owner; null after successful adoption. */
    FVM_SWFSoundMixer *mixer = 0; /* Owns only the current adopted replay. */
    FVM_SWFSoundVoiceInfo info; /* Explicit finite duration and effective source geometry. */
    FVM_SWFSoundVoiceStorageInfo storage; /* Exact retained template bytes. */
    FVM_SWFSoundMixerStorageInfo mixer_storage; /* Fixed mixer charge before every clone. */
    FVM_SWFSoundMixerConfig geometry; /* Same-clock native output, no source-rate conversion. */
    FILE *input = 0, *output = 0; /* Separate explicitly named file owners. */
    unsigned char *bytes = 0; /* Temporary input storage released after load. */
    long length = 0; /* Validate the signed ftell result before casting. */
    unsigned long duration = 0, total = 0, position = 0; /* Checked finite frame counts. */
    size_t available = 0, clone_budget = 0, frames = 0, written = 0; /* Requested native storage and scalar output. */
    short pcm[EXAMPLE_REUSE_BLOCK * EXAMPLE_REUSE_CHANNELS]; /* Disjoint caller transfer storage. */
    unsigned int pass = 0; /* Count independent replay owners, never duplicate decode calls. */
    int status = 1; /* Fail unless every read/write/close completes. */
    if (argc == 1) /* Usage has no VM or filesystem side effects. */
    { /* Explain the explicit output destination. */
        puts("Usage: swf_voice_reuse INPUT.swf OUTPUT.pcm"); /* Native-endian interleaved S16 output. */
        return 0; /* A usage request succeeds. */
    } /* End usage-only mode. */
    if (argc != 3) return 1; /* Reject malformed CLI before opening a file. */
    input = fopen(argv[1], "rb"); /* Acquire only the requested source. */
    if (!input || fseek(input, 0, SEEK_END) != 0) goto cleanup; /* Require a seekable regular input. */
    length = ftell(input); /* Determine the complete source size. */
    if (length <= 0 || (unsigned long)length > EXAMPLE_REUSE_INPUT_BYTES) goto cleanup; /* Bound source. */
    if (fseek(input, 0, SEEK_SET) != 0) goto cleanup; /* Restore the exact input start. */
    bytes = (unsigned char *)malloc((size_t)length); /* Acquire one bounded input copy. */
    if (!bytes || fread(bytes, 1, (size_t)length, input) != (size_t)length) goto cleanup; /* No partial load. */
    if (fclose(input) != 0) { input = 0; goto cleanup; } /* Preserve close failures. */
    input = 0; /* End file ownership once. */
    if (!checked(fvm_config_init(&config), "config")) goto cleanup; /* Initialize all fields. */
    config.memory_limit_bytes = EXAMPLE_REUSE_NATIVE_BYTES; /* Bound source runtime payloads. */
    if (!checked(fvm_runtime_create(&config, &runtime), "runtime")) goto cleanup; /* Own runtime. */
    if (!checked(fvm_context_create(runtime, &context), "context")) goto cleanup; /* Own source context. */
    if (!checked(fvm_load_memory(context, bytes, (size_t)length), "load")) goto cleanup; /* Copy source. */
    free(bytes); bytes = 0; /* No caller file bytes survive successful preparation. */
    if (!checked(fvm_swf_sound_voice_open_bounded(context, 0, /* Decode exactly once on the owner. */
        EXAMPLE_REUSE_NATIVE_BYTES, &source), "template")) goto cleanup; /* Retain native interval/style. */
    info.struct_size = sizeof(info); /* Request exact copied playback metadata. */
    if (!checked(fvm_swf_sound_voice_info(source, &info), "duration")) goto cleanup; /* Effective clock. */
    if (info.play_count && info.frames_per_play > /* Bound multiplication before computing duration. */
        EXAMPLE_REUSE_EXPORT_FRAMES / EXAMPLE_REUSE_PASSES / info.play_count) goto cleanup; /* Reject huge loops. */
    duration = (unsigned long)info.frames_per_play * info.play_count; /* One finite complete replay. */
    fvm_context_destroy(context); context = 0; /* No native owner borrows managed objects. */
    fvm_runtime_destroy(runtime); runtime = 0; /* Complete VM teardown before cloning. */
    storage.struct_size = sizeof(storage); /* Request native object/array charges. */
    if (!checked(fvm_swf_sound_voice_storage_info(source, &storage), "template bytes")) goto cleanup; /* No VM. */
    if (storage.retained_bytes > EXAMPLE_REUSE_NATIVE_BYTES) goto cleanup; /* Guard subtraction. */
    available = EXAMPLE_REUSE_NATIVE_BYTES - storage.retained_bytes; /* Reserve template ownership first. */
    geometry.struct_size = sizeof(geometry); /* Initialize exact configured mixer ABI. */
    geometry.sample_rate = info.sample_rate; geometry.channels = info.channels; /* Keep source geometry. */
    geometry.voice_capacity = EXAMPLE_REUSE_VOICES; /* Bound active replay count. */
    geometry.flags = FVM_SWF_SOUND_MIXER_CONVERT; /* Permit effective MPEG clocks without nominal-rate rejection. */
    if (!checked(fvm_swf_sound_mixer_create_bounded(&geometry, available, &mixer), "mixer")) goto cleanup; /* Own native mixer. */
    mixer_storage.struct_size = sizeof(mixer_storage); /* Query fixed object/slot accounting. */
    if (!checked(fvm_swf_sound_mixer_storage_info(mixer, &mixer_storage), "mixer bytes")) goto cleanup; /* No device. */
    if (mixer_storage.retained_bytes > available) goto cleanup; /* Guard the remaining clone allowance. */
    clone_budget = available - mixer_storage.retained_bytes; /* Include all simultaneously retained owners. */
    if (duration && !checked(fvm_swf_sound_voice_seek(source, info.play_count, 0), "template EOF")) goto cleanup; /* Keep completed template. */
    for (pass = 0; pass < EXAMPLE_REUSE_PASSES; ++pass) /* Each replay owns a fresh independent native cursor. */
    { /* Begin VM-free deep copy and transfer. */
        if (!checked(fvm_swf_sound_voice_clone(source, clone_budget, &copy), "clone")) goto cleanup; /* No decoder. */
        if (!checked(fvm_swf_sound_voice_seek(copy, 0, 0), "restart clone")) goto cleanup; /* Restore complete authored playback. */
        if (!checked(fvm_swf_sound_mixer_submit_prepared_bounded(mixer, &copy, available), "adopt")) goto cleanup; /* Consume only on OK. */
        if (!output) output = fopen(argv[2], "wb"); /* Replace requested output only after first successful adoption. */
        if (!output) goto cleanup; /* Terminal native file failure. */
        for (position = 0; position < duration; position += (unsigned long)frames) /* No infinite mixer silence. */
        { /* Begin finite caller-authorized PCM copy. */
            frames = (size_t)(duration - position); /* Remaining frames in this replay. */
            if (frames > EXAMPLE_REUSE_BLOCK) frames = EXAMPLE_REUSE_BLOCK; /* Bound native work. */
            if (!checked(fvm_swf_sound_mixer_read(mixer, frames, pcm, /* Advance only independent native data. */
                sizeof(pcm) / sizeof(pcm[0]), &written), "read")) goto cleanup; /* Scalar capacity/output. */
            if (fwrite(pcm, sizeof(pcm[0]), written, output) != written) goto cleanup; /* Short write is terminal. */
            total += (unsigned long)frames; /* Count only completely accepted file PCM. */
        } /* End this finite replay. */
    } /* End both independent native owners. */
    if (fclose(output) != 0) { output = 0; goto cleanup; } /* Observe final flush errors. */
    output = 0; /* End output ownership once. */
    printf("Exported %lu frames at %u Hz/%u channels in %u VM-free replays.\n", /* Native sample geometry. */
        total, info.sample_rate, info.channels, EXAMPLE_REUSE_PASSES); /* Explain exact expected output. */
    status = 0; /* Every explicit operation completed. */
cleanup: /* Quiesce all readers before releasing borrowed owners. */
    if (input) fclose(input); /* Release partial input acquisition. */
    if (output) fclose(output); /* Failure may leave partial output, explicitly documented. */
    free(bytes); /* Release any unconsumed temporary source copy. */
    fvm_swf_sound_voice_close(copy); /* Release rejected/untransferred clone only. */
    fvm_swf_sound_mixer_close(mixer); /* Release adopted clone after all reads stop. */
    fvm_swf_sound_voice_close(source); /* End template ownership after the last clone finishes. */
    if (context) fvm_context_destroy(context); /* Release partial managed preparation. */
    if (runtime) fvm_runtime_destroy(runtime); /* Release source runtime last if still present. */
    return status; /* Report every terminal preparation/output failure. */
} /* End deterministic native replay tutorial. */
