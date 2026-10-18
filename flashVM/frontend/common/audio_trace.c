/* Host audio diagnostics: no file I/O or allocations in the native PCM producer's hot path. */
#include "audio_trace.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FVM_TRACE_SECONDS 20U     /* Capture reserve covers a 15-second run and queued lookahead. */
#define FVM_TRACE_EVENTS 4096U    /* Bound callback/event history independently of PCM capacity. */
#define FVM_TRACE_PATH 4096U      /* Bound the explicitly supplied host output basename. */
#define FVM_TRACE_RIFF_HEADER 44U /* PCM WAV header size including the data subchunk. */
#define FVM_TRACE_SAMPLE_BYTES 2U /* Portable S16 little-endian output. */

/* One native operation, measured before shutdown file I/O can perturb timing. */
typedef struct FVM_TraceEvent
{
    Uint32 elapsed; /* SDL monotonic milliseconds since capture initialization, modulo Uint32. */
    const char *operation; /* Borrow a static operation name; no dynamic strings in worker calls. */
    unsigned long value;   /* Operation-specific frame count, event ordinal or pause flag. */
    FVM_Result result;     /* Actual backend/VM status, not inferred playback success. */
    size_t frame;          /* Accepted interleaved-frame cursor; clears do not rewind evidence. */
} FVM_TraceEvent;

/* Independent native storage, serialized externally with the player audio mutex. */
struct FVM_DesktopAudioTrace
{
    short *pcm;            /* Owned fixed-capacity S16 snapshot of accepted SDL-bound input. */
    size_t count;          /* Captured interleaved frames, not scalar sample count. */
    size_t capacity;       /* Immutable frame capacity. */
    size_t event_count;    /* Initialized operation records. */
    unsigned long dropped; /* Operations omitted after the fixed event table fills. */
    int truncated;         /* At least one accepted frame did not fit the bounded snapshot. */
    Uint32 started;        /* Initial SDL monotonic clock. */
    FVM_SDLAudioQueueInfo device; /* Immutable obtained geometry. */
    size_t target;                /* Player reserve in frames. */
    size_t resume;                /* Player resume threshold in frames. */
    char path[FVM_TRACE_PATH];    /* Copied explicit output basename, excluding suffix. */
    FVM_TraceEvent events[FVM_TRACE_EVENTS]; /* Fixed record slots. */
};

/* Write integer bytes explicitly, preserving WAV endianness on every supported host. */
static int put_le(FILE *file, unsigned long value, unsigned int bytes)
{
    unsigned int i;
    for (i = 0; i < bytes; ++i)
        if (fputc((int)((value >> (i * 8U)) & 255U), file) == EOF)
            return 0;
    return 1;
}

/* Publish only complete files; remove incomplete temporary output after a write failure. */
static int complete(FILE *file, const char *temporary, const char *destination, int ok)
{
    if (fclose(file) != 0)
        ok = 0;
    if (!ok)
    {
        remove(temporary);
        return 0;
    }
    /* C rename replaces on POSIX/Android; explicit removal also permits Windows replacement. */
    remove(destination);
    if (rename(temporary, destination) == 0)
        return 1;
    remove(temporary);
    return 0;
}

/* Environment opt-in keeps normal playback free of capture allocation, copying and disk writes. */
FVM_DesktopAudioTrace *fvm_desktop_trace_open(const FVM_SDLAudioQueueInfo *device, size_t target,
                                              size_t resume)
{
    const char *path = SDL_getenv("FVM_AUDIO_TRACE");
    FVM_DesktopAudioTrace *trace;
    if (!path || !*path)
        return 0;
    if (strlen(path) >= FVM_TRACE_PATH)
    {
        SDL_Log("FlashVM trace: output path too long; capture disabled");
        return 0;
    }
    trace = (FVM_DesktopAudioTrace *)calloc(1, sizeof(*trace));
    if (!trace)
        return 0;
    trace->capacity = (size_t)device->sample_rate * FVM_TRACE_SECONDS;
    trace->pcm = (short *)malloc(trace->capacity * device->channels * sizeof(short));
    if (!trace->pcm)
    {
        free(trace);
        SDL_Log("FlashVM trace: allocation failed; capture disabled");
        return 0;
    }
    strcpy(trace->path, path);
    trace->device = *device;
    trace->target = target;
    trace->resume = resume;
    trace->started = SDL_GetTicks();
    return trace;
}

/* No allocation, I/O or VM access: caller already holds the shared native producer mutex. */
void fvm_desktop_trace_event(FVM_DesktopAudioTrace *trace, const char *operation,
                             unsigned long value, FVM_Result result)
{
    FVM_TraceEvent *event;
    if (!trace)
        return;
    if (trace->event_count == FVM_TRACE_EVENTS)
    {
        ++trace->dropped;
        return;
    }
    event = &trace->events[trace->event_count++];
    event->elapsed = SDL_GetTicks() - trace->started;
    event->operation = operation;
    event->value = value;
    event->result = result;
    event->frame = trace->count;
}

/* Preserve native S16 input verbatim; capture truncation never rejects or modifies playback PCM. */
void fvm_desktop_trace_pcm(FVM_DesktopAudioTrace *trace, const short *pcm, size_t frames)
{
    size_t count;
    if (!trace)
        return;
    count = trace->capacity - trace->count;
    if (count > frames)
        count = frames;
    if (count < frames)
        trace->truncated = 1;
    memcpy(trace->pcm + trace->count * trace->device.channels, pcm,
           count * trace->device.channels * sizeof(short));
    trace->count += count;
}

/* Shutdown-only serialization; .wav is submitted PCM, not a microphone or hardware-loopback
 * recording. */
void fvm_desktop_trace_close(FVM_DesktopAudioTrace *trace)
{
    char destination[FVM_TRACE_PATH + 16], temporary[FVM_TRACE_PATH + 16];
    FILE *file;
    size_t i;
    unsigned long bytes;
    int ok;
    const char *driver;
    if (!trace)
        return;
    driver = SDL_GetCurrentAudioDriver();
    bytes = (unsigned long)(trace->count * trace->device.channels * FVM_TRACE_SAMPLE_BYTES);
    strcpy(destination, trace->path);
    strcat(destination, ".wav");
    strcpy(temporary, trace->path);
    strcat(temporary, ".wav.tmp");
    file = fopen(temporary, "wb");
    if (file)
    {
        ok = fwrite("RIFF", 1, 4, file) == 4 && put_le(file, bytes + FVM_TRACE_RIFF_HEADER - 8U, 4);
        ok = ok && fwrite("WAVEfmt ", 1, 8, file) == 8 && put_le(file, 16, 4) && put_le(file, 1, 2);
        ok = ok && put_le(file, trace->device.channels, 2) &&
             put_le(file, trace->device.sample_rate, 4);
        ok = ok &&
             put_le(file,
                    trace->device.sample_rate * trace->device.channels * FVM_TRACE_SAMPLE_BYTES, 4);
        ok = ok && put_le(file, trace->device.channels * FVM_TRACE_SAMPLE_BYTES, 2) &&
             put_le(file, 16, 2);
        ok = ok && fwrite("data", 1, 4, file) == 4 && put_le(file, bytes, 4);
        for (i = 0; ok && i < trace->count * trace->device.channels; ++i)
            ok = put_le(file, (unsigned long)(unsigned short)trace->pcm[i], 2);
        if (!complete(file, temporary, destination, ok))
            SDL_Log("FlashVM trace: WAV save failed: %s", destination);
    }
    else
        SDL_Log("FlashVM trace: cannot open WAV: %s", destination);
    strcpy(destination, trace->path);
    strcat(destination, ".txt");
    strcpy(temporary, trace->path);
    strcat(temporary, ".txt.tmp");
    file = fopen(temporary, "wb");
    if (file)
    {
        fprintf(file,
                "FlashVM native SDL-bound PCM evidence\nrate=%u channels=%u quantum=%u target=%lu "
                "resume=%lu\n",
                trace->device.sample_rate, trace->device.channels,
                trace->device.device_buffer_frames, (unsigned long)trace->target,
                (unsigned long)trace->resume);
        fprintf(file, "driver=%s frames=%lu truncated=%d dropped_events=%lu\n",
                driver ? driver : "none", (unsigned long)trace->count, trace->truncated,
                trace->dropped);
        fprintf(
            file,
            "WAV concatenates accepted blocks; clear/pause and device latency are NOT rendered.\n");
        fprintf(file, "ms operation value result captured_frame\n");
        for (i = 0; i < trace->event_count; ++i)
        {
            FVM_TraceEvent *event = &trace->events[i];
            fprintf(file, "%lu %s %lu %d %lu\n", (unsigned long)event->elapsed, event->operation,
                    event->value, (int)event->result, (unsigned long)event->frame);
        }
        ok = !ferror(file);
        if (!complete(file, temporary, destination, ok))
            SDL_Log("FlashVM trace: report save failed: %s", destination);
    }
    else
        SDL_Log("FlashVM trace: cannot open report: %s", destination);
    free(trace->pcm);
    free(trace);
}
