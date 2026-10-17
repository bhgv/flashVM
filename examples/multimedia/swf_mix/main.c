/* Tutorial: read two native voice snapshots in bounded chunks and mix into a real host PCM file. */
#include "flashvm/swf_sound.h" /* Import snapshot ownership and portable mixer/output APIs. */
#include "example_constants.h" /* Name independent voice count and bounded caller geometry. */
#include <stdio.h> /* Implement actual fwrite/fclose output without a hidden device. */
#include <string.h> /* Initialize copied metadata and host ownership safely. */
/* Keep native output lifetime independent of the VM's managed heap. */
typedef struct MixedFile /* Own only the explicitly selected host output. */
{ /* Begin host output ownership. */
    const char *path; /* Borrow argv storage through context cleanup. */
    FILE *file; /* Own a lazily opened real output stream. */
} MixedFile; /* Finish native ownership state. */
/* Consume a mixed borrowed native-S16 block synchronously. */
static FVM_Result write_mixed(void *raw, const FVM_MediaFrame *frame) /* Retain no PCM pointer after return. */
{ /* Begin actual native output. */
    MixedFile *host = (MixedFile *)raw; /* Recover host-owned FILE state. */
    if (!host->file) /* Do not create output until the first nonempty mixed block. */
        host->file = fopen(host->path, "wb"); /* Create or replace only the caller-selected path. */
    if (!host->file || fwrite(frame->data, 1, frame->size, host->file) != frame->size) /* Require the complete initialized mixed block. */
        return FVM_ERROR; /* Preserve an opened FILE for ordinary fallback cleanup. */
    return FVM_OK; /* Acknowledge synchronous consumption before caller buffers are reused. */
} /* Finish real native output. */
/* Close the actual output when the context's channel is explicitly stopped. */
static FVM_Result stop_mixed(void *raw) /* Keep retries safe after a native close error. */
{ /* Begin native stop ownership. */
    MixedFile *host = (MixedFile *)raw; /* Recover this independent output. */
    int result = 0; /* Treat an absent file as already stopped. */
    if (host->file) /* Close only an acquired native stream. */
    { /* Release the handle regardless of the native flush result. */
        result = fclose(host->file); /* Flush and close actual mixed PCM output. */
        host->file = 0; /* Prevent a retry from closing an invalid FILE. */
    } /* Finish acquired file cleanup. */
    return result == 0 ? FVM_OK : FVM_ERROR; /* Preserve real native I/O failure. */
} /* Finish output stopping. */
/* Report API errors with ordinary control flow, keeping partial cleanup available. */
static int checked(FVM_Result result, const char *operation) /* Use no tutorial assertions. */
{ /* Begin result reporting. */
    if (result == FVM_OK) /* Continue only after acknowledged success. */
        return 1; /* Let the caller proceed. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Identify the operation and exact result code. */
    return 0; /* Request normal centralized cleanup. */
} /* Finish result reporting. */
/* Use literal SWF8 bytes: voice A plays once, voice B plays twice at the same 44100 Hz clock. */
static const unsigned char swf[] = /* Keep the fixture reproducible without an external encoder or asset. */
{ /* Begin one exact 54-byte uncompressed root movie. */
    'F', 'W', 'S', 8, 54, 0, 0, 0, 8, 0, 0, 12, 1, 0, /* Encode FWS version/size, RECT, rate and one root frame. */
    0x8a, 3, 7, 0, 12, 3, 0, 0, 0, 0, 128, 255, /* DefineSound 7 yields S16 -32768, 0, 32512. */
    0x8a, 3, 8, 0, 12, 3, 0, 0, 0, 255, 128, 0, /* DefineSound 8 yields S16 32512, 0, -32768. */
    0xc3, 3, 7, 0, 0, /* StartSound ordinal 0 plays resource 7 once. */
    0xc5, 3, 8, 0, 4, 2, 0, /* StartSound ordinal 1 plays resource 8 twice. */
    0x40, 0, 0, 0 /* Finish the single declared frame and root stream. */
}; /* Finish literal voice inputs. */
/* Explicitly align snapshot reads, pad the shorter voice and mix before one host output. */
int main(int argc, char **argv) /* Keep the public integration valid C89. */
{ /* Begin independent VM, snapshot and native output ownership. */
    FVM_Config config; /* Initialize before runtime creation. */
    FVM_Runtime *runtime = 0; /* Own the runtime after successful creation. */
    FVM_Context *context = 0; /* Own canonical SWF and the registered output channel. */
    FVM_AudioChannel *channel = 0; /* Borrow a context-owned mixed output registration. */
    FVM_SWFSoundVoice *voices[TUTORIAL_MIX_INPUT_COUNT]; /* Own native snapshots explicitly, outside managed GC. */
    FVM_SWFSoundVoiceInfo info[TUTORIAL_MIX_INPUT_COUNT]; /* Copy each snapshot's immutable output geometry. */
    FVM_MediaFrame inputs[TUTORIAL_MIX_INPUT_COUNT], mixed; /* Borrow caller buffers only during mixing/output. */
    FVM_MediaOutput output; /* Copy actual host callbacks into the channel. */
    MixedFile host; /* Keep userdata alive until context cleanup returns. */
    short source[TUTORIAL_MIX_INPUT_COUNT][TUTORIAL_MIX_CHUNK_FRAMES * TUTORIAL_MIX_MAX_CHANNELS]; /* Bound every per-voice read. */
    short destination[TUTORIAL_MIX_CHUNK_FRAMES * TUTORIAL_MIX_MAX_CHANNELS]; /* Bound each complete summed output. */
    size_t i, written, frames, total = 0; /* Count channel values separately from shared sample frames. */
    FVM_Result result; /* Distinguish normal snapshot EOF from API failure. */
    int exit_code = 1; /* Report success only after all actual output stops. */
    memset(voices, 0, sizeof(voices)); /* Make native close safe after partial snapshot setup. */
    memset(inputs, 0, sizeof(inputs)); /* Clear unused video/timestamp fields before audio geometry is filled. */
    memset(&host, 0, sizeof(host)); /* Make fallback cleanup safe after any earlier failure. */
    if (argc != TUTORIAL_MIX_ARGUMENT_COUNT) /* Require an explicit output path before any file/device operation. */
    { /* Describe the real file replacement side effect. */
        printf("Usage: %s OUTPUT.pcm\nReplaces the named file with raw native S16 mixed output.\n", argv[0]); /* Keep no-argument smoke runs side-effect free. */
        return 0; /* No runtime, snapshot or host output has been acquired. */
    } /* Finish explicit output selection. */
    if (!checked(fvm_config_init(&config), "config_init") || !checked(fvm_runtime_create(&config, &runtime), "runtime_create")) /* Acquire VM ownership in dependency order. */
        goto cleanup; /* Release any acquired state normally. */
    if (!checked(fvm_context_create(runtime, &context), "context_create") || !checked(fvm_load_memory(context, swf, sizeof(swf)), "load_memory")) /* Retain canonical fixture bytes before preparation. */
        goto cleanup; /* No file has been opened. */
    for (i = 0; i < TUTORIAL_MIX_INPUT_COUNT; ++i) /* Prepare the fixture's two root controls independently. */
    { /* Each snapshot retains source PCM once regardless of its repetitions. */
        if (!checked(fvm_swf_sound_voice_open(context, i, &voices[i]), "voice_open")) /* Capture one source interval and envelope without activating output. */
            goto cleanup; /* Close earlier native snapshots through the shared path. */
        info[i].struct_size = sizeof(info[i]); /* Declare the exact copied-progress ABI. */
        if (!checked(fvm_swf_sound_voice_info(voices[i], &info[i]), "voice_info")) /* Copy rate/channel geometry without callbacks. */
            goto cleanup; /* Keep snapshot ownership explicit on failure. */
        if (info[i].channels > TUTORIAL_MIX_MAX_CHANNELS || (i && (info[i].sample_rate != info[0].sample_rate || info[i].channels != info[0].channels))) /* Require equal geometry rather than silently resampling or downmixing. */
        { /* Reject input combinations that need a separate conversion stage. */
            fprintf(stderr, "Voices need equal rates/channels within the caller buffer geometry.\n"); /* Explain the preparation requirement. */
            goto cleanup; /* Output remains unopened. */
        } /* Finish common geometry validation. */
        inputs[i].kind = FVM_MEDIA_AUDIO; /* Select native interleaved S16 input. */
        inputs[i].sample_rate = info[i].sample_rate; /* Retain the captured source clock. */
        inputs[i].channels = info[i].channels; /* Retain the captured layout. */
        inputs[i].data = (const unsigned char *)source[i]; /* Borrow this voice's caller-owned read buffer. */
    } /* Finish coherent input preparation. */
    host.path = argv[1]; /* Borrow the explicitly chosen mixed output path. */
    output.struct_size = sizeof(output); /* Supply the complete host callback ABI. */
    output.userdata = &host; /* Retain FILE ownership outside managed GC. */
    output.write = write_mixed; /* Delegate actual native output to fwrite. */
    output.stop = stop_mixed; /* Delegate output stopping to fclose. */
    if (!checked(fvm_audio_channel_create(context, &output, &channel), "channel_create")) /* Register an initially inactive mixed output. */
        goto cleanup; /* The file is still opened lazily by the first write. */
    for (;;) /* The host explicitly advances every voice on a shared sample clock. */
    { /* Prepare one bounded time-aligned block. */
        frames = 0; /* Choose the longest populated voice slice, padding shorter ones with silence. */
        for (i = 0; i < TUTORIAL_MIX_INPUT_COUNT; ++i) /* Read each independent source at the same logical output offset. */
        { /* EOF is normal and contributes a zero-byte view. */
            result = fvm_swf_sound_voice_read(voices[i], TUTORIAL_MIX_CHUNK_FRAMES, source[i], sizeof(source[i]) / sizeof(source[i][0]), &written); /* Read without allocation or VM source access. */
            if (result != FVM_NOT_FOUND && !checked(result, "voice_read")) /* Preserve errors while accepting normal end-of-voice. */
                goto cleanup; /* Earlier read positions have advanced; no hidden replay occurs. */
            inputs[i].samples = written / info[i].channels; /* Convert initialized channel values to sample frames. */
            inputs[i].size = written * sizeof(short); /* Declare zero bytes for an exhausted voice. */
            if (inputs[i].samples > frames) /* Retain the longest actual slice for the final partial block. */
                frames = inputs[i].samples; /* The mixer supplies silence for shorter slices. */
        } /* Finish every source's aligned read. */
        if (!frames) /* Stop when all snapshots report EOF together. */
            break; /* Submit no empty output frame. */
        if (!checked(fvm_audio_mix_s16(inputs, TUTORIAL_MIX_INPUT_COUNT, info[0].sample_rate, info[0].channels, frames, destination, sizeof(destination) / sizeof(destination[0]), &mixed), "mix_s16")) /* Sum all sources exactly, then saturate once into caller storage. */
            goto cleanup; /* Failed validation changes neither output PCM nor metadata. */
        if (!checked(fvm_audio_channel_write(channel, &mixed), "mixed_write")) /* Consume borrowed mixed bytes synchronously through the actual host file bridge. */
            goto cleanup; /* Report native failure without replaying already advanced voices. */
        total += frames; /* Count accepted output frames rather than summing input durations. */
    } /* Finish all time-aligned blocks. */
    if (!checked(fvm_audio_channel_stop(channel), "mixed_stop")) /* Close the actual output before reporting success. */
        goto cleanup; /* Context teardown can retry a failed stop safely. */
    printf("Mixed %lu frames; expected six native S16 frames, no audible playback.\n", (unsigned long)total); /* Expose observable file output and shared duration. */
    exit_code = 0; /* Every mixed block and real native stop succeeded. */
cleanup: /* Preserve host userdata until all context callbacks finish. */
    for (i = 0; i < TUTORIAL_MIX_INPUT_COUNT; ++i) /* Close every acquired native snapshot independently. */
        fvm_swf_sound_voice_close(voices[i]); /* Null handles are safe; no context/output callback is entered. */
    if (context) /* Destroy only a successfully acquired context. */
        fvm_context_destroy(context); /* Stop registered output before host userdata leaves scope. */
    if (host.file) /* Recover a FILE left outside active state by a failed native write. */
        fclose(host.file); /* Release fallback native ownership after context callbacks return. */
    if (runtime) /* Runtime must outlive its context. */
        fvm_runtime_destroy(runtime); /* Release the remaining VM ownership last. */
    return exit_code; /* Use ordinary status instead of tutorial assertions. */
} /* Finish explicit bounded source reading, mixing and host output. */
