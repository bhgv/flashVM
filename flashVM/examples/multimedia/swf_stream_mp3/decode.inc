/* Decode one real SWF segment with bounded host input and detached native PCM ownership. */
#include "decode_constants.h" /* Import explicit host-input and native chunk bounds. */
/* Run on the VM owner until capture, then use only native storage and a caller-owned file. */
static int decode_file(const char *input, const char *output) /* Borrow explicit user paths. */
{ /* Initialize every owner before fallible work. */
    FILE *source = 0, *target = 0; /* Own only successfully opened host files. */
    unsigned char *wire = 0; /* Own the bounded input copy until loading completes. */
    long length = 0; /* Receive the seekable input extent before converting to size_t. */
    FVM_Config config; /* Configure the VM before creating its runtime. */
    FVM_Runtime *runtime = 0; FVM_Context *context = 0; /* Own the temporary VM graph. */
    FVM_SWFStreamSoundSnapshot *snapshot = 0; /* Own detached normalized S16 after capture. */
    FVM_SWFStreamSoundInfo info; FVM_SWFStreamPCMReader reader; /* Keep source geometry and position. */
    FVM_SWFStreamPCMChunk chunk; /* Receive frame/block metadata without a managed reference. */
    short pcm[TUTORIAL_MP3_VALUES]; /* Reuse fixed caller output storage after each write. */
    size_t written = 0, frames = 0; FVM_Result result; /* Publish only acknowledged decoded data. */
    int status = 1; /* Preserve failure until all reads and the final flush succeed. */
    source = fopen(input, "rb"); /* Open only the explicitly selected input. */
    if (!source || fseek(source, 0, SEEK_END) || (length = ftell(source)) <= 0 || (unsigned long)length > TUTORIAL_MP3_BYTES || fseek(source, 0, SEEK_SET)) goto cleanup; /* Reject empty, oversized or unseekable input. */
    wire = (unsigned char *)malloc((size_t)length); /* Allocate only the previously bounded input extent. */
    if (!wire || fread(wire, 1, (size_t)length, source) != (size_t)length || fgetc(source) != EOF || ferror(source)) goto cleanup; /* Reject short or changing input. */
    if (fclose(source)) { source = 0; goto cleanup; } /* Release the input even after a close error. */
    source = 0; /* Prevent a second close in cleanup. */
    if (fvm_config_init(&config) != FVM_OK) goto cleanup; /* Establish the current public configuration ABI. */
    config.memory_limit_bytes = TUTORIAL_MP3_BYTES; /* Bound VM preparation as well as the host input. */
    if (fvm_runtime_create(&config, &runtime) != FVM_OK || fvm_context_create(runtime, &context) != FVM_OK || fvm_load_memory(context, wire, (size_t)length) != FVM_OK) goto cleanup; /* Acquire and load the temporary source graph. */
    free(wire); wire = 0; /* Loaded SWF storage no longer borrows the host input allocation. */
    if (fvm_swf_stream_sound_snapshot_open_pcm_bounded(context, 0, TUTORIAL_MP3_BYTES, &snapshot) != FVM_OK) goto cleanup; /* Normalize and validate the entire first segment before opening output. */
    fvm_context_destroy(context); context = 0; /* Detach playback from the source managed graph. */
    fvm_runtime_destroy(runtime); runtime = 0; /* Keep only native snapshot storage alive. */
    info.struct_size = sizeof(info); /* Declare the exact copied metadata ABI. */
    if (fvm_swf_stream_sound_snapshot_info(snapshot, &info) != FVM_OK || fvm_swf_stream_pcm_reader_init(&reader, snapshot) != FVM_OK) goto cleanup; /* Borrow the immutable prepared source. */
    if (output) { /* Verification mode deliberately needs no destination file. */
        target = fopen(output, "wb"); /* Replace only the explicit output after successful preparation. */
        if (!target) goto cleanup; /* Leave the validated snapshot for centralized cleanup. */
    } /* Keep native reading identical in verification and export modes. */
    for (;;) { /* Successful empty markers still advance through the source index. */
        chunk.struct_size = sizeof(chunk); /* Declare the output chunk ABI before each read. */
        result = fvm_swf_stream_pcm_reader_next(&reader, TUTORIAL_MP3_VALUES / info.channels, pcm, TUTORIAL_MP3_VALUES, &chunk, &written); /* Read bounded native S16 without VM or allocation. */
        if (result == FVM_NOT_FOUND) break; /* EOF publishes no additional data. */
        if (result != FVM_OK || (target && fwrite(pcm, sizeof(short), written, target) != written)) goto cleanup; /* Reject decode or physical file-write errors. */
        frames += chunk.frames; /* Count decoded frames separately from channel values. */
    } /* Finish complete sequential extraction. */
    if (target && fclose(target)) { target = 0; goto cleanup; } /* Report final flush errors and invalidate ownership. */
    target = 0; /* Cleanup must not close a released output. */
    printf("%s %lu frames at %u Hz, %u channels\n", output ? "Decoded" : "Verified", (unsigned long)frames, info.sample_rate, info.channels); /* Identify the raw native-endian S16 geometry. */
    status = 0; /* All operations, including the final output flush, succeeded. */
cleanup: /* Release native owners in every success and failure path. */
    if (target) fclose(target); /* Preserve a partial output for host diagnosis after failure. */
    if (source) fclose(source); /* Release an input acquired before an earlier failure. */
    free(wire); /* Null is valid when loading already released the host copy. */
    fvm_swf_stream_sound_snapshot_close(snapshot); /* End all native reader borrows. */
    if (context) fvm_context_destroy(context); /* Release a partially prepared managed source. */
    if (runtime) fvm_runtime_destroy(runtime); /* Release only an acquired temporary runtime. */
    if (status) fputs("SWF stream decoding failed\n", stderr); /* Keep native failures visible without assertions. */
    return status; /* Report actual completion to the invoking host. */
} /* End detached bounded PCM export. */
