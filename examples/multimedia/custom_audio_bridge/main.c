/* Tutorial: adapt a real stdio PCM source to the replaceable multimedia ABI. */
#include "flashvm/multimedia.h" /* Import C89 bridge tables and borrowed frame views. */
#include <stdio.h> /* Open and read a real host file without VM file objects. */
#include <stdlib.h> /* Allocate private stream state with ordinary error handling. */
#include <string.h> /* Initialize callback tables and result views. */
#define TUTORIAL_PCM_SAMPLE_RATE 8000 /* Explicit sample-rate contract of the raw input format. */
#define TUTORIAL_PCM_BUFFER_SAMPLES 256 /* Maximum sample frames borrowed by one read. */
typedef struct PcmSource /* Own native resources independently of VM garbage collection. */
{ /* Begin the custom stream layout. */
    FILE *file; /* Own the explicitly requested host file until close. */
    short samples[TUTORIAL_PCM_BUFFER_SAMPLES]; /* Keep one borrowed mono S16 buffer alive between
                                                   reads. */
} PcmSource; /* Finish custom stream state. */
/* Open a raw 8000 Hz mono signed-16 native-endian file through the real C library. */
static FVM_Result pcm_open(void *host, const FVM_MediaRequest *request, /* Borrow request strings only during this callback. */
                           void **out) /* Borrow request strings only during this callback. */
{ /* Begin backend-specific opening. */
    PcmSource *source; /* Track a private stream that will become session-owned. */
    (void)host; /* This backend requires no shared application state. */
    *out = 0; /* Publish no partial native stream. */
    if (request->kind != FVM_MEDIA_AUDIO) /* Reject video and camera requests honestly. */
        return FVM_UNSUPPORTED; /* Reject video and camera requests honestly. */
    source = (PcmSource *)malloc(sizeof(*source)); /* Allocate real unmanaged state. */
    if (!source) /* Report allocation failure before opening a file. */
        return FVM_OUT_OF_MEMORY; /* Report allocation failure before opening a file. */
    source->file = fopen(request->url, "rb"); /* Call the actual host C library. */
    if (!source->file) /* Clean allocation if the host cannot open the requested file. */
    { /* Begin partial-open cleanup. */
        free(source); /* The failing callback owns cleanup of its partial state. */
        return FVM_ERROR; /* The session wrapper will not call close after a failed open. */
    } /* Finish failed-open cleanup. */
    *out = source; /* Transfer native ownership to the successful session. */
    return FVM_OK; /* Report a valid custom source. */
} /* Finish the open callback. */
/* Read one buffer; require complete signed-16 samples rather than ignoring a trailing byte. */
static FVM_Result pcm_read(void *opaque, FVM_MediaFrame *out) /* The common API has already cleared the result view. */
{ /* Begin reading the native source. */
    PcmSource *source = (PcmSource *)opaque; /* Recover the owned file and reusable buffer. */
    size_t bytes = /* Read accessible bytes through stdio. */
        fread(source->samples, 1, sizeof(source->samples), source->file); /* Read accessible bytes through stdio. */
    if (ferror(source->file) || bytes % sizeof(short)) /* Reject I/O errors and truncated PCM sample values. */
        return FVM_ERROR; /* Reject I/O errors and truncated PCM sample values. */
    if (!bytes) /* Return documented EOF without stale metadata. */
        return FVM_NOT_FOUND; /* Return documented EOF without stale metadata. */
    out->kind = FVM_MEDIA_AUDIO; /* Identify packed audio output. */
    out->data = (const unsigned char *)source->samples; /* Borrow storage until the next session operation. */
    out->size = bytes; /* Expose only initialized file bytes. */
    out->sample_rate = TUTORIAL_PCM_SAMPLE_RATE; /* Declare this tutorial's explicit raw-file sample rate. */
    out->channels = 1; /* Declare mono interleaving. */
    out->samples = bytes / sizeof(short); /* Report sample frames rather than bytes. */
    return FVM_OK; /* No playback device is opened by decoding. */
} /* Finish the read callback. */
/* Release the source exactly once after a successful open. */
static void pcm_close(void *opaque) /* The common session owns this callback invocation. */
{ /* Begin native cleanup. */
    PcmSource *source = (PcmSource *)opaque; /* Recover the native ownership wrapper. */
    fclose(source->file); /* Close the real host file; production backends may report close
                             diagnostics separately. */
    free(source); /* Release the unmanaged reusable buffer and wrapper. */
} /* Finish the close callback. */
/* Register a minimal custom backend and consume a host-selected raw PCM file. */
int main(int argc, char **argv) /* No arguments prints usage without accessing files/devices. */
{ /* Begin tutorial execution. */
    FVM_MediaBackend backend; /* Store the replaceable callback table. */
    FVM_MediaSession *session = 0; /* Own a successfully opened session until common close. */
    FVM_MediaFrame frame; /* Borrow one decoded audio buffer at a time. */
    FVM_Result result; /* Preserve read/open failures for ordinary error reporting. */
    size_t samples = 0; /* Count consumed sample frames without retaining borrowed pointers. */
    if (argc != 2) /* Require an explicit source rather than opening a hidden default. */
    { /* Explain the tutorial's raw-format contract. */
        puts("Usage: custom_audio_bridge input-native-s16-mono-8000.raw"); /* Describe the only
                                                                              required argument. */
        return argc == 1 ? 0 : 1; /* Keep no-argument smoke execution harmless. */
    } /* Finish usage handling. */
    memset(&backend, 0, sizeof(backend)); /* Leave unsupported optional callbacks null. */
    backend.struct_size = sizeof(backend); /* Identify this exact callback-table layout. */
    backend.abi_version = FVM_MEDIA_ABI_VERSION; /* Select the supported bridge ABI. */
    backend.os_name = "ISO C stdio / host OS"; /* Label the real host library used by this implementation. */
    backend.open = pcm_open; /* Connect native file creation. */
    backend.read = pcm_read; /* Connect native PCM reads. */
    backend.close = pcm_close; /* Connect deterministic resource release. */
    result = fvm_audio_open(&backend, argv[1], &session); /* Copy the callback table and open the real input. */
    if (result != FVM_OK) /* Report open errors without tutorial assertions. */
    { /* Keep failure output concise. */
        fprintf(stderr, "Open failed: %d\n", (int)result); /* Print the public result category. */
        return 1; /* No session exists to close on failed open. */
    } /* Finish open failure handling. */
    while ((result = fvm_media_read(session, &frame)) == FVM_OK) /* Decode until EOF or a real error. */
    { /* Consume the borrowed view before the next read. */
        samples += frame.samples; /* Keep only a scalar summary, never the borrowed pointer. */
    } /* Finish synchronous consumption. */
    fvm_media_close(session); /* Call pcm_close and release the common wrapper. */
    if (result != FVM_NOT_FOUND) /* Distinguish ordinary EOF from malformed input. */
    { /* Report the failed read after deterministic cleanup. */
        fprintf(stderr, "Read failed: %d\n", (int)result); /* Preserve the bridge's error category. */
        return 1; /* Do not present partial consumption as full success. */
    } /* Finish error reporting. */
    printf("Consumed %lu mono sample frames; no playback requested.\n", /* Describe the actual demonstrated operation. */
           (unsigned long)samples); /* Describe the actual demonstrated operation. */
    return 0; /* Finish the custom-source tutorial successfully. */
} /* Finish main. */
