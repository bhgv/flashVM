/* Inspect encoded MP3 prefixes or export validated native PCM from a real SWF without playback. */
#include "flashvm/swf_stream_pcm.h" /* Import the live and detached C89 inspection APIs. */
#include <stdlib.h> /* Own the bounded external-input buffer. */
#include <stdio.h> /* Print copied metadata and failures. */
#include <string.h> /* Recognize the explicit demonstration argument. */
#include "decode.inc" /* Add the annotated real-file normalization/export workflow. */
static const unsigned char fixture[] = { /* FWS4: two blocks with deliberately opaque MPEG data. */
    70, 87, 83, 4, 43, 0, 0, 0, /* Synthetic wire bytes; MPEG bytes are opaque, not playable audio. */
    8, 0, 0, 12, 2, 0, 134, 4, /* Synthetic wire bytes; MPEG bytes are opaque, not playable audio. */
    6, 46, 64, 2, 254, 255, 199, 4, /* Synthetic wire bytes; MPEG bytes are opaque, not playable audio. */
    64, 2, 253, 255, 255, 0, 126, 64, /* Synthetic wire bytes; MPEG bytes are opaque, not playable audio. */
    0, 196, 4, 0, 0, 0, 0, 64, /* Synthetic wire bytes; MPEG bytes are opaque, not playable audio. */
    0, 0, 0 /* Synthetic wire bytes; MPEG bytes are opaque, not playable audio. */
}; /* End the synthetic metadata fixture; this is not an MP3 codec acceptance sample. */
/* Print one copied prefix without accessing data after its owning snapshot is closed. */
static void print_block(const FVM_SWFStreamSoundMP3Block *block) /* Borrow the view synchronously. */
{ /* Begin metadata formatting. */
    printf("frame=%u declared=%u seek=%d bytes=%lu\n", /* Show SWF frames separately from declared audio samples. */
           block->frame, block->declared_samples, block->seek_samples, /* Preserve the signed seek adjustment. */
           (unsigned long)block->data_size); /* Exclude the four-byte prefix from the MPEG byte count. */
} /* Finish without taking ownership of the borrowed data. */
/* Prepare one segment, destroy its source VM, then inspect independent encoded storage. */
int main(int argc, char **argv) /* Require an explicit request to run the embedded fixture. */
{ /* Begin the tutorial's resource ownership. */
    FVM_Config config; /* Initialize before creating the runtime. */
    FVM_Runtime *runtime = 0; /* Keep partial initialization safe for cleanup. */
    FVM_Context *context = 0; /* Own the source movie until capture completes. */
    FVM_SWFStreamSoundSnapshot *snapshot = 0; /* Own encoded storage independently of VM lifetime. */
    FVM_SWFStreamSoundMP3Block block; /* Receive metadata plus a borrowed MPEG span. */
    FVM_SWFStreamSoundInfo info; /* Header latency remains separate from per-block seek. */
    size_t count = 0; /* Receive the number of encoded block occurrences. */
    size_t ordinal; /* Enumerate blocks in their original movie-frame order. */
    int status = 1; /* Change to success only after every operation succeeds. */
    if (argc == 3 && strcmp(argv[1], "--verify") == 0) return decode_file(argv[2], 0); /* Validate native PCM without creating any output file. */
    if (argc == 4 && strcmp(argv[1], "--decode") == 0) return decode_file(argv[2], argv[3]); /* Decode only explicitly supplied input/output paths. */
    if (argc == 1) /* Explain usage without allocating a VM or opening any file. */
    { /* Begin usage-only mode. */
        puts("usage: swf_stream_mp3 --demo | --verify INPUT.swf | --decode INPUT.swf OUTPUT.pcm"); /* Name the only accepted tutorial option. */
        return 0; /* No resources require cleanup. */
    } /* Finish usage-only mode. */
    if (argc != 2 || strcmp(argv[1], "--demo") != 0) /* Reject other arguments before side effects. */
        return 1; /* Report invalid use without initialization. */
    if (fvm_config_init(&config) != FVM_OK) /* Set the public configuration ABI fields. */
        goto cleanup; /* Preserve the common error cleanup path. */
    if (fvm_runtime_create(&config, &runtime) != FVM_OK) /* Acquire the runtime owner. */
        goto cleanup; /* Leave null owners untouched. */
    if (fvm_context_create(runtime, &context) != FVM_OK) /* Acquire the source context. */
        goto cleanup; /* Destroy the runtime if context creation failed. */
    if (fvm_load_memory(context, fixture, sizeof(fixture)) != FVM_OK) /* Load canonical native SWF bytes. */
        goto cleanup; /* Do not inspect an unsuccessful load. */
    block.struct_size = sizeof(block); /* Initialize the MP3 output ABI size. */
    if (fvm_swf_stream_sound_mp3_block_at(context, 0, 0, &block) != FVM_OK) /* Inspect the original live prefix. */
        goto cleanup; /* Keep malformed or unsupported input visible. */
    if (fvm_swf_stream_sound_snapshot_open(context, 0, &snapshot) != FVM_OK) /* Copy/index the encoded segment. */
        goto cleanup; /* No partially prepared snapshot is published. */
    fvm_context_destroy(context); /* Release the source movie after capture. */
    context = 0; /* Prevent the cleanup path from destroying it twice. */
    fvm_runtime_destroy(runtime); /* The snapshot retains no pointer to this owner. */
    runtime = 0; /* Continue with native independent storage only. */
    info.struct_size = sizeof(info); /* Initialize the copied source-header ABI. */
    if (fvm_swf_stream_sound_snapshot_info(snapshot, &info) != FVM_OK) /* Read the preserved stream header. */
        goto cleanup; /* Treat lookup failure as a failed tutorial. */
    printf("header latency=%d\n", info.latency_seek); /* Never silently add header latency to block seek. */
    if (fvm_swf_stream_sound_snapshot_block_count(snapshot, &count) != FVM_OK) /* Query the prepared index. */
        goto cleanup; /* No movie rescan or VM is needed. */
    for (ordinal = 0; ordinal < count; ++ordinal) /* Enumerate even a block with no MPEG bytes. */
    { /* Begin one immutable prefix lookup. */
        if (fvm_swf_stream_sound_snapshot_mp3_block_at(snapshot, ordinal, &block) != FVM_OK) /* Borrow until close. */
            goto cleanup; /* Reject truncated selected prefixes. */
        print_block(&block); /* Consume the copied metadata while its payload owner lives. */
    } /* Finish block enumeration. */
    status = 0; /* Every independent operation succeeded. */
cleanup: /* Release owned resources after success or any failure. */
    fvm_swf_stream_sound_snapshot_close(snapshot); /* Null is accepted; borrowed MPEG views end here. */
    if (context) /* Only a partially prepared run still owns a context. */
        fvm_context_destroy(context); /* Destroy it before its runtime. */
    if (runtime) /* Only release a runtime that is still owned. */
        fvm_runtime_destroy(runtime); /* No snapshot retains this pointer. */
    if (status) /* Explain an unsuccessful operation without assertions. */
        fputs("SWF MP3 metadata example failed\n", stderr); /* Keep failures observable in Release builds. */
    return status; /* Return ordinary process success/failure. */
} /* End the metadata-only tutorial. */
