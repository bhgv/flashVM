/* Independent bounded storage fixture: known signed samples, truncation and event overflow. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/audio_trace.h"
#include <SDL.h>
#include <string.h>


int main(int argc, char **argv)
{
    FVM_SDLAudioQueueInfo device;
    FVM_DesktopAudioTrace *trace;
    short pcm[400];
    unsigned int i;
    if (!(argc == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&device, 0, sizeof(device));
    device.struct_size = sizeof(device);
    device.sample_rate = 8; /* Deliberately small internal fixture: capacity is 160 frames. */
    device.channels = 2;
    device.device_buffer_frames = 1;
    if (!(SDL_setenv("FVM_AUDIO_TRACE", "", 1) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!fvm_desktop_trace_open(&device, 4, 2)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_desktop_trace_close(0);
    if (!(SDL_setenv("FVM_AUDIO_TRACE", argv[1], 1) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    trace = fvm_desktop_trace_open(&device, 4, 2);
    if (!(trace))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 200; ++i)
    {
        pcm[i * 2] = 0x1234;
        pcm[i * 2 + 1] = -2;
    }
    fvm_desktop_trace_pcm(trace, pcm, 100);
    fvm_desktop_trace_pcm(trace, pcm + 200, 100);
    for (i = 0; i < 4100; ++i)
    {
        fvm_desktop_trace_event(trace, "fixture", i, FVM_OK);
    }
    fvm_desktop_trace_close(trace);
    return 0;
}
