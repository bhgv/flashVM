/* Inspect encoded MP3 prefixes or export validated native PCM from a real SWF without playback. */
#include "flashvm/swf_stream_pcm.h" /* Import the live and detached C89 inspection APIs. */
#include <stdlib.h>                 /* Own the bounded external-input buffer. */
#include <stdio.h>                  /* Print copied metadata and failures. */
#include <string.h>                 /* Recognize the explicit demonstration argument. */
#include <errno.h>                  /* Detect numeric frame overflow without accepting partial input. */
#include <limits.h>                 /* Bound frame numbers to the public unsigned-int ABI. */
#include "decode.inc"               /* Add the annotated real-file normalization/export workflow. */

static const unsigned char fixture[] = { /* FWS4: two blocks with deliberately opaque MPEG data. */
    /* FWS4: two blocks with deliberately opaque MPEG data. */
    70, 87, 83, 4, 43, 0, 0, 0,       /* Synthetic wire bytes; MPEG bytes are opaque, not playable audio. */
    8, 0, 0, 12, 2, 0, 134, 4,        /* Synthetic wire bytes; MPEG bytes are opaque, not playable audio. */
    6, 46, 64, 2, 254, 255, 199, 4,   /* Synthetic wire bytes; MPEG bytes are opaque, not playable audio. */
    64, 2, 253, 255, 255, 0, 126, 64, /* Synthetic wire bytes; MPEG bytes are opaque, not playable audio. */
    0, 196, 4, 0, 0, 0, 0, 64,        /* Synthetic wire bytes; MPEG bytes are opaque, not playable audio. */
    0, 0, 0                           /* Synthetic wire bytes; MPEG bytes are opaque, not playable audio. */
}; /* End the synthetic metadata fixture; this is not an MP3 codec acceptance sample. */


/* Print one copied prefix without accessing data after its owning snapshot is closed. */
static void print_block(const FVM_SWFStreamSoundMP3Block *block)       /* Borrow the view synchronously. */
{                                                                      /* Begin metadata formatting. */
    printf("frame=%u declared=%u seek=%d bytes=%lu\n",                 /* Show SWF frames separately from declared audio samples. */
           block->frame, block->declared_samples, block->seek_samples, /* Preserve the signed seek adjustment. */
           (unsigned long)block->data_size);                           /* Exclude the four-byte prefix from the MPEG byte count. */
} /* Finish without taking ownership of the borrowed data. */


/* Accept only a positive complete decimal movie frame, preserving output on failure. */
static int parse_frame(const char *text, unsigned int *out) /* Borrow the CLI text for this call only. */
{                                                           /* Validate syntax before strtoul can accept signs or whitespace. */
    const char *at = text;                                  /* Keep C89 declarations at block start. */
    unsigned long value;                                    /* Keep C89 declarations at block start. */
    char *end = 0;                                          /* Keep C89 declarations at block start. */
    if (!text || !*text)                                    /* Condition for this action: Reject absent and empty arguments. */
    {                                                       /* Begin conditional branch: Condition for this action: Reject absent and empty arguments. */
        return 0;                                           /* Reject absent and empty arguments. */
    } /* End conditional branch: Condition for this action: Reject absent and empty arguments. */
    while (*at)                     /* Require decimal digits throughout. */
    {                               /* Begin loop body: Require decimal digits throughout. */
        if (*at < '0' || *at > '9') /* Condition for this action: Require decimal digits throughout. */
        {                           /* Begin conditional branch: Condition for this action: Require decimal digits throughout. */
            return 0;               /* Require decimal digits throughout. */
        } /* End conditional branch: Condition for this action: Require decimal digits throughout. */
        ++at; /* Require decimal digits throughout. */
    } /* Require decimal digits throughout. */
    errno = 0;                                                 /* Convert with a checked host range. */
    value = strtoul(text, &end, 10);                           /* Convert with a checked host range. */
    if (errno == ERANGE || *end || !value || value > UINT_MAX) /* Condition for this action: Reject zero and overflow. */
    {                                                          /* Begin conditional branch: Condition for this action: Reject zero and overflow. */
        return 0;                                              /* Reject zero and overflow. */
    } /* End conditional branch: Condition for this action: Reject zero and overflow. */
    *out = (unsigned int)value; /* Publish only a completely valid movie frame. */
    return 1;                   /* Publish only a completely valid movie frame. */
} /* No file or VM side effect occurs while parsing. */


/* Prepare one segment, destroy its source VM, then inspect independent encoded storage. */
int main(int argc, char **argv)                        /* Require an explicit request to run the embedded fixture. */
{                                                      /* Begin the tutorial's resource ownership. */
    FVM_Config config;                                 /* Initialize before creating the runtime. */
    FVM_Runtime *runtime = 0;                          /* Keep partial initialization safe for cleanup. */
    FVM_Context *context = 0;                          /* Own the source movie until capture completes. */
    FVM_SWFStreamSoundSnapshot *snapshot = 0;          /* Own encoded storage independently of VM lifetime. */
    FVM_SWFStreamSoundMP3Block block;                  /* Receive metadata plus a borrowed MPEG span. */
    FVM_SWFStreamSoundInfo info;                       /* Header latency remains separate from per-block seek. */
    size_t count = 0;                                  /* Receive the number of encoded block occurrences. */
    size_t ordinal;                                    /* Enumerate blocks in their original movie-frame order. */
    unsigned int seek_frame = 0;                       /* Receive a validated optional one-based seek frame. */
    int status = 1;                                    /* Change to success only after every operation succeeds. */
    if (argc == 3 && strcmp(argv[1], "--verify") == 0) /* Condition for this action: Validate native PCM without creating any output file. */
    {                                                  /* Begin conditional branch: Condition for this action: Validate native PCM without creating any output file. */
        return decode_file(argv[2], 0, 0);             /* Validate native PCM without creating any output file. */
    } /* End conditional branch: Condition for this action: Validate native PCM without creating any output file. */
    if (argc == 4 && strcmp(argv[1], "--decode") == 0) /* Condition for this action: Decode only explicitly supplied input/output paths. */
    {                                                  /* Begin conditional branch: Condition for this action: Decode only explicitly supplied input/output paths. */
        return decode_file(argv[2], argv[3], 0);       /* Decode only explicitly supplied input/output paths. */
    } /* End conditional branch: Condition for this action: Decode only explicitly supplied input/output paths. */
    if (argc == 5 && strcmp(argv[1], "--seek") == 0) /* Condition for this action: Require explicit source, frame and destination. */
    {                                                /* Require explicit source, frame and destination. */
        if (!parse_frame(argv[3], &seek_frame))      /* Condition for this action: Reject malformed input before file access. */
        {                                            /* Begin conditional branch: Condition for this action: Reject malformed input before file access. */
            return 1;                                /* Reject malformed input before file access. */
        } /* End conditional branch: Condition for this action: Reject malformed input before file access. */
        return decode_file(argv[2], argv[4], seek_frame); /* Validate then export the post-VM PCM suffix. */
    } /* Other options retain their existing argument validation. */
    if (argc == 1)                                                                                                                     /* Explain usage without allocating a VM or opening any file. */
    {                                                                                                                                  /* Begin usage-only mode. */
        puts("usage: swf_stream_mp3 --demo | --verify INPUT.swf | --decode INPUT.swf OUTPUT.pcm | --seek INPUT.swf FRAME OUTPUT.pcm"); /* Name the only accepted tutorial option. */
        return 0;                                                                                                                      /* No resources require cleanup. */
    } /* Finish usage-only mode. */
    if (argc != 2 || strcmp(argv[1], "--demo") != 0) /* Reject other arguments before side effects. */
    {                                                /* Begin conditional branch: Reject other arguments before side effects. */
        return 1;                                    /* Report invalid use without initialization. */
    } /* End conditional branch: Reject other arguments before side effects. */
    if (fvm_config_init(&config) != FVM_OK) /* Set the public configuration ABI fields. */
    {                                       /* Begin conditional branch: Set the public configuration ABI fields. */
        goto cleanup;                       /* Preserve the common error cleanup path. */
    } /* End conditional branch: Set the public configuration ABI fields. */
    if (fvm_runtime_create(&config, &runtime) != FVM_OK) /* Acquire the runtime owner. */
    {                                                    /* Begin conditional branch: Acquire the runtime owner. */
        goto cleanup;                                    /* Leave null owners untouched. */
    } /* End conditional branch: Acquire the runtime owner. */
    if (fvm_context_create(runtime, &context) != FVM_OK) /* Acquire the source context. */
    {                                                    /* Begin conditional branch: Acquire the source context. */
        goto cleanup;                                    /* Destroy the runtime if context creation failed. */
    } /* End conditional branch: Acquire the source context. */
    if (fvm_load_memory(context, fixture, sizeof(fixture)) != FVM_OK) /* Load canonical native SWF bytes. */
    {                                                                 /* Begin conditional branch: Load canonical native SWF bytes. */
        goto cleanup;                                                 /* Do not inspect an unsuccessful load. */
    } /* End conditional branch: Load canonical native SWF bytes. */
    block.struct_size = sizeof(block);                                      /* Initialize the MP3 output ABI size. */
    if (fvm_swf_stream_sound_mp3_block_at(context, 0, 0, &block) != FVM_OK) /* Inspect the original live prefix. */
    {                                                                       /* Begin conditional branch: Inspect the original live prefix. */
        goto cleanup;                                                       /* Keep malformed or unsupported input visible. */
    } /* End conditional branch: Inspect the original live prefix. */
    if (fvm_swf_stream_sound_snapshot_open(context, 0, &snapshot) != FVM_OK) /* Copy/index the encoded segment. */
    {                                                                        /* Begin conditional branch: Copy/index the encoded segment. */
        goto cleanup;                                                        /* No partially prepared snapshot is published. */
    } /* End conditional branch: Copy/index the encoded segment. */
    fvm_context_destroy(context);                                      /* Release the source movie after capture. */
    context = 0;                                                       /* Prevent the cleanup path from destroying it twice. */
    fvm_runtime_destroy(runtime);                                      /* The snapshot retains no pointer to this owner. */
    runtime = 0;                                                       /* Continue with native independent storage only. */
    info.struct_size = sizeof(info);                                   /* Initialize the copied source-header ABI. */
    if (fvm_swf_stream_sound_snapshot_info(snapshot, &info) != FVM_OK) /* Read the preserved stream header. */
    {                                                                  /* Begin conditional branch: Read the preserved stream header. */
        goto cleanup;                                                  /* Treat lookup failure as a failed tutorial. */
    } /* End conditional branch: Read the preserved stream header. */
    printf("header latency=%d\n", info.latency_seek);                          /* Never silently add header latency to block seek. */
    if (fvm_swf_stream_sound_snapshot_block_count(snapshot, &count) != FVM_OK) /* Query the prepared index. */
    {                                                                          /* Begin conditional branch: Query the prepared index. */
        goto cleanup;                                                          /* No movie rescan or VM is needed. */
    } /* End conditional branch: Query the prepared index. */
    for (ordinal = 0; ordinal < count; ++ordinal)                                            /* Enumerate even a block with no MPEG bytes. */
    {                                                                                        /* Begin one immutable prefix lookup. */
        if (fvm_swf_stream_sound_snapshot_mp3_block_at(snapshot, ordinal, &block) != FVM_OK) /* Borrow until close. */
        {                                                                                    /* Begin conditional branch: Borrow until close. */
            goto cleanup;                                                                    /* Reject truncated selected prefixes. */
        } /* End conditional branch: Borrow until close. */
        print_block(&block); /* Consume the copied metadata while its payload owner lives. */
    } /* Finish block enumeration. */
    status = 0;                                    /* Every independent operation succeeded. */
cleanup:                                           /* Release owned resources after success or any failure. */
    fvm_swf_stream_sound_snapshot_close(snapshot); /* Null is accepted; borrowed MPEG views end here. */
    if (context)                                   /* Only a partially prepared run still owns a context. */
    {                                              /* Begin conditional branch: Only a partially prepared run still owns a context. */
        fvm_context_destroy(context);              /* Destroy it before its runtime. */
    } /* End conditional branch: Only a partially prepared run still owns a context. */
    if (runtime)                      /* Only release a runtime that is still owned. */
    {                                 /* Begin conditional branch: Only release a runtime that is still owned. */
        fvm_runtime_destroy(runtime); /* No snapshot retains this pointer. */
    } /* End conditional branch: Only release a runtime that is still owned. */
    if (status)                                             /* Explain an unsuccessful operation without assertions. */
    {                                                       /* Begin conditional branch: Explain an unsuccessful operation without assertions. */
        fputs("SWF MP3 metadata example failed\n", stderr); /* Keep failures observable in Release builds. */
    } /* End conditional branch: Explain an unsuccessful operation without assertions. */
    return status; /* Return ordinary process success/failure. */
} /* End the metadata-only tutorial. */
