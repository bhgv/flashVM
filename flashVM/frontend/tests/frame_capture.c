/* Real owner rasters, exact callback order, paused refresh and copied pixels surviving SDK
 * teardown. */
#ifdef FVM_TEST_OFFSCREEN
#define SDL_MAIN_HANDLED
#include <SDL.h>
#define fvm_desktop_run_source_observed fvm_desktop_run_source_offscreen
#define fvm_desktop_run_source_background_observed fvm_desktop_run_source_background_offscreen
#endif
#include "../common/player.h"
#ifdef FVM_DESKTOP_SOCKET_HTTP
#include "../common/socket_http.h"
#elif defined(FVM_DESKTOP_HTTP)
#include "../common/http.h"
#endif
#include "flashvm/stage.h"
#include "flashvm/gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Native test storage never retains callback-owned pixels, text or managed objects. */
typedef struct Capture
{
    const char *scenario;
    unsigned char copy[32 * 32 * 4], previous[32 * 32 * 4];
    char source[FVM_DESKTOP_MAX_LOCATION + 1];
    unsigned long count, last_tick;
    int failed, dirty, received, cancelled;
} Capture;
/* Validate one exact packed RGBA pixel without endian-dependent integer casts. */
static int pixel(const unsigned char *data, unsigned int x, unsigned int r, unsigned int g,
                 unsigned int b)
{
    const unsigned char *p = data + (8 * 32 + x) * 4;
    return p[0] == r && p[1] == g && p[2] == b && p[3] == 255;
}
/* Copy within the borrow, then compare metadata and visible pixels against independent
 * expectations. */
static FVM_Result capture_frame(const FVM_DesktopFrame *frame, void *raw)
{
    Capture *test = (Capture *)raw;
    unsigned long tick;
    int orange;
#ifdef FVM_TEST_OFFSCREEN
    if (SDL_WasInit(SDL_INIT_VIDEO))
        return FVM_ERROR;
#endif
    ++test->count;
    if (!frame || frame->struct_size != sizeof(*frame) ||
        frame->format != FVM_DESKTOP_FRAME_RGBA8 || frame->width != 32 || frame->height != 32 ||
        frame->stride != 128 || frame->byte_count != sizeof(test->copy) || !frame->source ||
        frame->total_presentations != test->count ||
        fvm_desktop_frame_copy(frame, test->copy, sizeof(test->copy), 128) != FVM_OK)
        goto failed;
    strcpy(test->source, frame->source);
    test->last_tick = frame->total_ticks;
    if (!strcmp(test->scenario, "sequence"))
    {
        static const unsigned long ticks[] = {0, 1, 1, 2};
        static const unsigned int reasons[] = {FVM_DESKTOP_FRAME_REFRESH, FVM_DESKTOP_FRAME_TICK,
                                               FVM_DESKTOP_FRAME_REFRESH, FVM_DESKTOP_FRAME_TICK};
        if (test->count > 4 || frame->movie_ticks != ticks[test->count - 1] ||
            frame->total_ticks != frame->movie_ticks || frame->presentations != test->count ||
            frame->reason != reasons[test->count - 1])
            goto failed;
        if (test->count == 1)
        {
            if (!pixel(test->copy, 8, 255, 255, 255) || !pixel(test->copy, 22, 255, 255, 255))
                goto failed;
            return FVM_OK;
        }
        if (test->count == 2)
            memcpy(test->previous, test->copy, sizeof(test->copy));
        if (test->count == 3 && memcmp(test->previous, test->copy, sizeof(test->copy)))
            goto failed;
    }
    else if (!strcmp(test->scenario, "navigation"))
    {
        if (frame->reason != FVM_DESKTOP_FRAME_TICK || frame->movie_ticks != 1 ||
            frame->presentations != 1 || frame->total_ticks != test->count ||
            (test->count == 2 && !strstr(frame->source, "next.swf")))
            goto failed;
    }
    else if (frame->reason != FVM_DESKTOP_FRAME_TICK || frame->movie_ticks != test->count ||
             frame->total_ticks != test->count || frame->presentations != test->count)
        goto failed;
    tick = frame->total_ticks;
    orange = !(tick & 1UL);
    if (orange ? (!pixel(test->copy, 8, 255, 255, 255) || !pixel(test->copy, 22, 255, 128, 64))
               : (!pixel(test->copy, 8, 64, 128, 255) || !pixel(test->copy, 22, 255, 255, 255)))
        goto failed;
    if (!strcmp(test->scenario, "fail") ||
        (!strcmp(test->scenario, "fail-active") && test->received && tick >= 2))
        return FVM_ERROR;
    return FVM_OK;
failed:
    fprintf(stderr, "frame fixture: invalid record or pixel at observation %lu\n", test->count);
    test->failed = 1;
    return FVM_ERROR;
}
/* [GC:] Only the controlled owner queries/collects a pinned live root, never the frame observer. */
static FVM_DesktopPlaybackCommand control_frame(const FVM_DesktopPlaybackState *state, void *raw)
{
    Capture *test = (Capture *)raw;
    if (fvm_gc_collect(state->context, 0) != FVM_OK)
    {
        test->failed = 1;
        return FVM_DESKTOP_PLAYBACK_STOP;
    }
    if (!strcmp(test->scenario, "navigation"))
        return state->total_ticks >= 2 ? FVM_DESKTOP_PLAYBACK_STOP : FVM_DESKTOP_PLAYBACK_STEP;
    if (state->phase == FVM_DESKTOP_PLAYBACK_TICK)
        return test->count >= 4 ? FVM_DESKTOP_PLAYBACK_STOP : FVM_DESKTOP_PLAYBACK_PAUSE;
    if (test->count == 1 || test->count == 3)
        return FVM_DESKTOP_PLAYBACK_STEP;
    if (test->count == 2 && !test->dirty)
    {
        test->dirty = 1;
        if (fvm_stage_request_update(state->context) != FVM_OK)
        {
            test->failed = 1;
            return FVM_DESKTOP_PLAYBACK_STOP;
        }
    }
    return FVM_DESKTOP_PLAYBACK_KEEP;
}
/* Observe immutable transfer progress/cancellation while callbacks own only native userdata. */
static int loading(const FVM_DesktopLoadEvent *event, void *raw)
{
    Capture *test = (Capture *)raw;
    if (event->kind == FVM_DESKTOP_LOAD_VARIABLES && event->received)
        test->received = 1;
    if (event->kind == FVM_DESKTOP_LOAD_VARIABLES && event->phase == FVM_DESKTOP_LOAD_CANCELLED)
        test->cancelled = 1;
    return 0;
}
/* Keep exact borrowed startup bytes alive through the complete source run. */
static unsigned char *read_bytes(const char *path, size_t *size)
{
    FILE *f = fopen(path, "rb");
    long length;
    unsigned char *bytes = 0;
    if (!f)
        return 0;
    if (!fseek(f, 0, SEEK_END) && (length = ftell(f)) > 0 && !fseek(f, 0, SEEK_SET))
    {
        bytes = (unsigned char *)malloc((size_t)length);
        if (bytes && fread(bytes, 1, (size_t)length, f) == (size_t)length)
            *size = (size_t)length;
        else
        {
            free(bytes);
            bytes = 0;
        }
    }
    fclose(f);
    return bytes;
}
int main(int argc, char **argv)
{
    Capture test;
    FVM_DesktopOptions options;
    FVM_DesktopSource source;
    FVM_DesktopLoader loader;
    FVM_DesktopFrameObserver observer;
    FVM_DesktopPlaybackControl control;
    FVM_DesktopBackgroundLoader background;
    FVM_DesktopLoadMonitor monitor;
    unsigned char *bytes = 0;
    size_t size = 0;
    const char *error = 0;
    int status, network, controlled;
#ifdef FVM_TEST_OFFSCREEN
    SDL_Event host_event;
    Uint32 host_subsystems;
#endif
    if (argc < 4 || argc > 5)
        return 2;
    memset(&test, 0, sizeof(test));
    test.scenario = argv[3];
    network = strstr(argv[1], "://") != 0;
    controlled = !strcmp(test.scenario, "sequence") || !strcmp(test.scenario, "navigation");
    fvm_desktop_options_init(&options);
    options.mode = !strcmp(argv[2], "--jit") ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
    options.mute = 1;
    options.stats = 1;
    options.frames = !strcmp(test.scenario, "fail-active") ? 100 : 2;
    options.fps = 100;
    fvm_desktop_frame_observer_init(&observer);
    observer.notify = capture_frame;
    observer.userdata = &test;
    fvm_desktop_playback_control_init(&control);
    control.initially_paused = 1;
    control.notify = control_frame;
    control.userdata = &test;
    fvm_desktop_loader_init(&loader);
    loader.max_loads = 8;
    fvm_desktop_background_loader_init(&background);
    background.max_loads = 8;
#ifdef FVM_DESKTOP_SOCKET_HTTP
    background.transfer.fetch = fvm_desktop_socket_transfer_fetch;
#elif defined(FVM_DESKTOP_HTTP)
    background.transfer.fetch = fvm_desktop_http_transfer_fetch;
#endif
    if (network && !background.transfer.fetch)
        return 77;
    fvm_desktop_load_monitor_init(&monitor);
    monitor.notify = loading;
    monitor.userdata = &test;
    memset(&source, 0, sizeof(source));
    source.location = argv[1];
    source.kind = network ? FVM_DESKTOP_SOURCE_URL : FVM_DESKTOP_SOURCE_FILE;
    if (argc == 5)
    {
        bytes = read_bytes(argv[4], &size);
        if (!bytes)
            return 3;
        source.kind = FVM_DESKTOP_SOURCE_MEMORY;
        source.bytes = bytes;
        source.size = size;
    }
#ifdef FVM_TEST_OFFSCREEN
    SDL_SetMainReady();
    if (SDL_InitSubSystem(SDL_INIT_TIMER | SDL_INIT_EVENTS) != 0)
        return 4;
    host_subsystems = SDL_WasInit(0);
    memset(&host_event, 0, sizeof(host_event));
    host_event.type = SDL_USEREVENT;
    host_event.user.code = 720;
    if (SDL_PushEvent(&host_event) != 1)
        return 4;
#endif
    if (!strcmp(test.scenario, "bad"))
        observer.notify = 0;
    if (network)
        status = fvm_desktop_run_source_background_observed(
            &options, &source, &background, &monitor, controlled ? &control : 0, &observer, &error);
    else
        status = fvm_desktop_run_source_observed(&options, &source, &loader,
                                                 controlled ? &control : 0, &observer, &error);
    free(bytes);
    if (!strcmp(test.scenario, "bad"))
    {
        if (status != 2 || test.count)
            test.failed = 1;
    }
    else if (!strcmp(test.scenario, "fail") || !strcmp(test.scenario, "fail-active"))
    {
        if (status != 1 || !test.count ||
            (!strcmp(test.scenario, "fail-active") && !test.cancelled))
            test.failed = 1;
    }
    else if (status || test.count != (!strcmp(test.scenario, "sequence") ? 4UL : 2UL))
        test.failed = 1;
#ifdef FVM_TEST_OFFSCREEN
    if (SDL_WasInit(0) != host_subsystems || SDL_PollEvent(&host_event) != 1 ||
        host_event.type != SDL_USEREVENT || host_event.user.code != 720)
        test.failed = 1;
    SDL_QuitSubSystem(SDL_INIT_TIMER | SDL_INIT_EVENTS);
#endif
    if (fvm_desktop_shutdown() != FVM_OK)
        test.failed = 1;
    if (strcmp(test.scenario, "bad") &&
        !pixel(test.copy, (test.last_tick & 1UL) ? 8 : 22, (test.last_tick & 1UL) ? 64 : 255, 128,
               (test.last_tick & 1UL) ? 255 : 64))
        test.failed = 1;
    printf("capture count=%lu ticks=%lu cancelled=%d failed=%d\n", test.count, test.last_tick,
           test.cancelled, test.failed);
    return test.failed ? 1 : 0;
}
