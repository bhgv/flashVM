/* Tutorial: inspect root streaming PCM blocks and send bounded copies through a real stdio bridge. */
#include "flashvm/swf_stream_sound.h" /* Import stream metadata, PCM conversion and channel output. */
#include "example_constants.h" /* Name the explicit output and bounded caller geometry. */
#include <stdio.h> /* Use actual fopen/fwrite/fclose calls in the replaceable output bridge. */
#include <string.h> /* Clear borrowed media views before setting audio fields. */
/* Keep native file ownership alive through every context cleanup callback. */
typedef struct StreamFile /* Own the real output independently of the managed graph. */
{ /* Begin file bridge state. */
    const char *path; /* Borrow argv storage until main returns. */
    FILE *file; /* Own the lazily opened native stream. */
} StreamFile; /* Finish host-owned bridge state. */
/* Consume one initialized native-S16 block synchronously. */
static FVM_Result write_pcm(void *raw, const FVM_MediaFrame *frame) /* Never retain the borrowed PCM pointer. */
{ /* Begin actual file output. */
    StreamFile *host = (StreamFile *)raw; /* Recover host-owned userdata. */
    if (!host->file) /* Open only after a successful nonempty PCM decode. */
        host->file = fopen(host->path, "wb"); /* Replace only the explicitly selected output. */
    if (!host->file || fwrite(frame->data, 1, frame->size, host->file) != frame->size) /* Require synchronous complete consumption. */
        return FVM_ERROR; /* Leave any acquired handle for ordinary cleanup. */
    return FVM_OK; /* Allow reuse of the caller buffer after return. */
} /* Finish native PCM delivery. */
/* Flush and close the actual stream while preserving retry-safe ownership. */
static FVM_Result stop_pcm(void *raw) /* Use the same callback for explicit and terminal stopping. */
{ /* Begin real output stop. */
    StreamFile *host = (StreamFile *)raw; /* Recover the independently owned output. */
    int status = 0; /* An unopened or already closed stream needs no native work. */
    if (host->file) /* Close only an acquired native FILE. */
    { /* Invalidate ownership even if flushing fails. */
        status = fclose(host->file); /* Report an actual native close/flush failure. */
        host->file = 0; /* Prevent duplicate fclose during a later retry. */
    } /* Finish release of an acquired stream. */
    return status ? FVM_ERROR : FVM_OK; /* Keep the error visible to the VM caller. */
} /* Finish output stopping. */
/* Print ordinary API errors and preserve centralized cleanup. */
static int checked(FVM_Result result, const char *operation) /* Borrow only a diagnostic label. */
{ /* Begin ordinary status handling. */
    if (result == FVM_OK) /* Continue after acknowledged success only. */
        return 1; /* Let the caller proceed. */
    fprintf(stderr, "%s failed: %d\n", operation, (int)result); /* Identify the failing API and result. */
    return 0; /* Request normal ownership cleanup. */
} /* Finish error handling without assertions. */
/* Independent FWS4 fixture: one Head2, variable PCM blocks, a frame gap and an empty block. */
static const unsigned char movie[] = /* Keep the wire input reproducible without an encoder. */
{ /* Begin one exact 42-byte root movie. */
    'F', 'W', 'S', 4, 42, 0, 0, 0, 8, 0, 0, 12, 4, 0, /* Declare four movie frames at 12 fps. */
    0x44, 0x0b, 6, 0x3d, 1, 0, /* Advise mono 11025 Hz but encode stereo 44100 Hz PCM with average one. */
    0xc4, 4, 0, 255, 128, 128, 0x40, 0, /* Frame one holds two stereo frames, not the advisory average. */
    0x40, 0, /* Frame two has no sound block; do not invent silence automatically. */
    0xc2, 4, 255, 0, 0x40, 0, /* Frame three holds one stereo sample pair. */
    0xc0, 4, 0x40, 0, 0, 0 /* Frame four holds an explicit empty block, then the movie ends. */
}; /* Finish independent wire bytes. */
/* Enumerate blocks, decode small ranges and feed the ordinary registered output. */
int main(int argc, char **argv) /* Use the public C89 ABI without assertions or hidden devices. */
{ /* Begin explicit VM and host ownership. */
    FVM_Config config; /* Initialize before acquiring runtime ownership. */
    FVM_Runtime *runtime = 0; /* Own runtime after successful creation. */
    FVM_Context *context = 0; /* Own canonical FWS bytes and channel registrations. */
    FVM_SWFStreamSoundSnapshot *snapshot = 0; /* Own optional prepared source independently of both VMs. */
    FVM_AudioChannel *channel = 0; /* Borrow the context-owned registration. */
    FVM_SWFStreamSoundInfo info; /* Copy one governing stream header. */
    FVM_SWFStreamSoundBlock block; /* Borrow payload metadata only while the current SWF is loaded. */
    FVM_MediaOutput output; /* Copy actual file callbacks into the registered channel. */
    FVM_MediaFrame frame; /* Borrow initialized caller PCM during each synchronous write. */
    StreamFile host; /* Keep output userdata alive through context destruction. */
    short samples[TUTORIAL_STREAM_CHUNK_FRAMES * TUTORIAL_STREAM_CHANNELS]; /* Bound caller-owned native-S16 output. */
    size_t streams, blocks, ordinal, first, frames, written, total = 0; /* Distinguish ordinals, sample frames and channel values. */
    int prepared = argc == TUTORIAL_STREAM_ARGUMENT_COUNT + 1 && !strcmp(argv[2], "--prepared"); /* Select explicit native indexing before any side effects. */
    int exit_code = 1; /* Report success only after actual output stopping. */
    memset(&host, 0, sizeof(host)); /* Make partial-setup and failed-write cleanup safe. */
    if (argc != TUTORIAL_STREAM_ARGUMENT_COUNT && !prepared) /* Require an explicit replaceable output path. */
    { /* Keep no-argument smoke runs side-effect free. */
        printf("Usage: %s OUTPUT.pcm [--prepared]\nReplaces the selected file with raw native S16 stereo PCM.\n", argv[0]); /* Describe the actual file side effect. */
        return 0; /* No VM, file or device ownership exists yet. */
    } /* Finish explicit output selection. */
    host.path = argv[1]; /* Borrow the caller-selected path. */
    if (!checked(fvm_config_init(&config), "config_init") || !checked(fvm_runtime_create(&config, &runtime), "runtime_create")) /* Acquire runtime in dependency order. */
        goto cleanup; /* Release any successfully acquired owner. */
    if (!checked(fvm_context_create(runtime, &context), "context_create") || !checked(fvm_load_memory(context, movie, sizeof(movie)), "load_memory")) /* Store canonical decoded movie bytes. */
        goto cleanup; /* No output file has been opened. */
    if (!checked(fvm_swf_stream_sound_count(context, &streams), "stream_count")) /* Validate the complete root audio control stream. */
        goto cleanup; /* Reject malformed later controls before native output. */
    info.struct_size = sizeof(info); /* Declare the exact copied-header ABI. */
    if (!streams || !checked(fvm_swf_stream_sound_at(context, 0, &info), "stream_at") || !checked(fvm_swf_stream_sound_block_count(context, 0, &blocks), "block_count")) /* Select the fixture's first header segment. */
        goto cleanup; /* Keep missing/header errors visible. */
    if (prepared) /* Demonstrate that the index does not borrow the original VM. */
    { /* Prepare before destroying source ownership. */
        if (!checked(fvm_swf_stream_sound_snapshot_open(context, 0, &snapshot), "snapshot_open")) /* Copy source bytes and block index once. */
            goto cleanup; /* Preserve ordinary allocation errors. */
        fvm_context_destroy(context); /* Release canonical movie storage. */
        context = 0; /* Keep partial reacquisition cleanup safe. */
        fvm_runtime_destroy(runtime); /* Prove no runtime pointer is retained by the snapshot. */
        runtime = 0; /* Mark the old owner released. */
        if (!checked(fvm_runtime_create(&config, &runtime), "output_runtime") || !checked(fvm_context_create(runtime, &context), "output_context")) /* Create an unloaded VM only for output registration. */
            goto cleanup; /* The independent snapshot still has its own cleanup. */
        if (!checked(fvm_swf_stream_sound_snapshot_info(snapshot, &info), "snapshot_info") || !checked(fvm_swf_stream_sound_snapshot_block_count(snapshot, &blocks), "snapshot_count")) /* Obtain copied metadata after original VM teardown. */
            goto cleanup; /* No movie has been loaded into the output VM. */
    } /* Finish independent source preparation. */
    if (info.channels != TUTORIAL_STREAM_CHANNELS) /* Require the layout promised by the bounded stereo buffer. */
        goto cleanup; /* Do not silently reinterpret channel geometry. */
    output.struct_size = sizeof(output); /* Supply the complete host bridge ABI. */
    output.userdata = &host; /* Retain native ownership outside the VM heap. */
    output.write = write_pcm; /* Use real fwrite for every submitted block. */
    output.stop = stop_pcm; /* Use real fclose when playback is explicitly stopped. */
    if (!checked(fvm_audio_channel_create(context, &output, &channel), "channel_create")) /* Copy the bridge table into an inactive context registration. */
        goto cleanup; /* The output remains unopened. */
    for (ordinal = 0; ordinal < blocks; ++ordinal) /* Visit encoded block occurrences, including explicit empty blocks. */
    { /* Preserve movie frame numbers independently of sample offsets. */
        block.struct_size = sizeof(block); /* Declare the exact borrowed-block view ABI. */
        if (!checked(snapshot ? fvm_swf_stream_sound_snapshot_block_at(snapshot, ordinal, &block) : fvm_swf_stream_sound_block_at(context, 0, ordinal, &block), "block_at")) /* Re-resolve current canonical bytes. */
            goto cleanup; /* Report metadata failure before reading payload. */
        printf("Frame %u: %lu sample frames\n", block.frame, (unsigned long)block.sample_count); /* Expose the gap and variable block lengths. */
        if (!block.sample_count_known) /* Compressed stream payloads have no supported PCM count here. */
            goto cleanup; /* Never present opaque bytes as decoded samples. */
        for (first = 0; first < block.sample_count; first += frames) /* Read source-relative ranges within this block. */
        { /* Keep every native output chunk bounded by caller capacity. */
            frames = block.sample_count - first; /* Count remaining sample frames, not scalar channel values. */
            if (frames > TUTORIAL_STREAM_CHUNK_FRAMES) /* Split a long block without a full-block allocation. */
                frames = TUTORIAL_STREAM_CHUNK_FRAMES; /* Retain fixed caller geometry. */
            if (!checked(snapshot ? fvm_swf_stream_sound_snapshot_read_pcm(snapshot, ordinal, first, frames, samples, sizeof(samples) / sizeof(samples[0]), &written) : fvm_swf_stream_sound_read_pcm(context, 0, ordinal, first, frames, samples, sizeof(samples) / sizeof(samples[0]), &written), "read_pcm")) /* Convert into independent native shorts without callbacks. */
                goto cleanup; /* Preserve actual decoder/range errors. */
            memset(&frame, 0, sizeof(frame)); /* Clear scheduling/video fields; this tutorial does no real-time scheduling. */
            frame.kind = FVM_MEDIA_AUDIO; /* Select the existing native-S16 output bridge. */
            frame.data = (const unsigned char *)samples; /* Borrow caller-owned bytes only during write. */
            frame.size = written * sizeof(samples[0]); /* Publish initialized channel values only. */
            frame.sample_rate = info.sample_rate; /* Use stream geometry rather than advisory playback geometry. */
            frame.channels = info.channels; /* Retain source stereo interleaving. */
            frame.samples = frames; /* Count complete source sample pairs. */
            if (!checked(fvm_audio_channel_write(channel, &frame), "channel_write")) /* Deliver actual decoded bytes to the registered file callback. */
                goto cleanup; /* Do not retry delivered host side effects implicitly. */
            total += frames; /* Count accepted source sample frames. */
        } /* Finish this block's bounded native copies. */
    } /* Finish encoded blocks without synthesizing missing-frame silence. */
    if (!checked(fvm_audio_stop_all(context), "stop_all")) /* Flush and close the actual native output through its registration. */
        goto cleanup; /* Terminal cleanup can retry an acknowledged-active failed stop. */
    printf("Exported %lu sample frames from %lu blocks.\n", (unsigned long)total, (unsigned long)blocks); /* Expect three stereo frames from three encoded blocks. */
    exit_code = 0; /* All parsing, decoding, host writes and explicit stopping succeeded. */
cleanup: /* Preserve userdata until all context-owned callbacks finish. */
    if (context) /* Destroy only an acquired context. */
        fvm_context_destroy(context); /* Stop remaining active output before host state leaves scope. */
    if (host.file) /* Recover a native FILE left by a failed write outside acknowledged active state. */
        fclose(host.file); /* Release fallback ownership after context callbacks return. */
    if (runtime) /* Runtime must outlive its context. */
        fvm_runtime_destroy(runtime); /* Release remaining VM ownership last. */
    fvm_swf_stream_sound_snapshot_close(snapshot); /* Release native source storage independently; null is accepted. */
    return exit_code; /* Report ordinary success/failure rather than using tutorial assertions. */
} /* Finish explicit root-stream export with no automatic movie clock. */
