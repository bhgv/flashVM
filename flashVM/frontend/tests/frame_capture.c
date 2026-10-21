/* Real owner rasters, exact callback order, paused refresh and copied pixels surviving SDK
 * teardown. */
#ifdef FVM_TEST_OFFSCREEN
#define SDL_MAIN_HANDLED
#include <SDL.h>
#define fvm_desktop_run_source_observed fvm_desktop_run_source_offscreen
#define fvm_desktop_run_source_background_observed fvm_desktop_run_source_background_offscreen
#endif
#include "../common/player.h"
#ifdef FVM_TEST_RESOURCE_CONTROL
#include "../common/resource_control.h"
#include "flashvm/rwire.h"
#ifdef FVM_TEST_RESOURCE_9P
#include "../common/resource_state.h"
#include "../common/resource_frame.h"
#include "resource_9p_client.h"
#endif
#endif
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
#ifdef FVM_TEST_RESOURCE_CONTROL
    FVM_RDispatch *dispatch;
    FVM_DesktopResourceControl adapter;
    FVM_SBindings incoming;
    FVM_SBinding types[1];
    unsigned long requests;
#ifdef FVM_TEST_RESOURCE_9P
    FVM_DesktopResourceState *state_adapter;
    FVM_RHandle state_target;
    FVM_DesktopResourceFrame *frame_adapter;
    FVM_RHandle frame_target;
    FVM_DesktopResourceFrameInfo frame_info[2];
    unsigned char held_pixels[2][32 * 32 * 4];
    Test9PClient clients[2];
    unsigned long epochs[2], state_events[2], ticks_seen[2];
#endif
#endif
    unsigned char copy[32 * 32 * 4], previous[32 * 32 * 4];
    char source[FVM_DESKTOP_MAX_LOCATION + 1];
    unsigned long count, last_tick;
    int failed, dirty, received, cancelled;
} Capture;
#ifdef FVM_TEST_RESOURCE_9P
/* Construct canonical operation arguments, independent of native byte layout. */
static size_t frame_number(unsigned char *out, unsigned long value)
{
    FVM_SInteger number;
    size_t used = 0;
    number.high = 0;
    number.low = value;
    number.negative = 0;
    if (fvm_sinteger_write(&number, out, FVM_SRECORD_INTEGER_MAX, &used) != FVM_S_OK)
        return 0;
    return used;
}
/* Reassemble a whole leased image from bounded 9P operation replies and compare every RGBA byte. */
static int frame_pixels(Capture *test, size_t client, const unsigned char *expected)
{
    unsigned char bytes[64], pixels[32 * 32 * 4];
    size_t offset = 0, n;
    FVM_RReply reply;
    FVM_DesktopResourceFrameInfo *info = test->frame_info + client;
    while (offset < sizeof(pixels))
    {
        n = frame_number(bytes, info->lease.id);
        n += frame_number(bytes + n, info->lease.generation);
        n += frame_number(bytes + n, (unsigned long)offset);
        n += frame_number(bytes + n, FVM_R_BYTES);
        if (!test_9p_call(test->clients + client, test->dispatch, test->frame_target,
                          FVM_DESKTOP_RFRAME_CHUNK, bytes, n, &reply) ||
            reply.size != FVM_R_BYTES)
            return 0;
        memcpy(pixels + offset, reply.data, reply.size);
        offset += reply.size;
    }
    if (memcmp(pixels, expected, sizeof(pixels)))
        return 0;
    /* Nonzero x and multiple rows prove packed regions skip the source row suffix. */
    n = frame_number(bytes, info->lease.id);
    n += frame_number(bytes + n, info->lease.generation);
    n += frame_number(bytes + n, 7);
    n += frame_number(bytes + n, 5);
    n += frame_number(bytes + n, 16);
    n += frame_number(bytes + n, 8);
    if (!test_9p_call(test->clients + client, test->dispatch, test->frame_target,
                      FVM_DESKTOP_RFRAME_REGION, bytes, n, &reply) ||
        reply.size != FVM_R_BYTES)
        return 0;
    for (offset = 0; offset < 8; ++offset)
        if (memcmp(reply.data + offset * 64, expected + ((offset + 5) * 32 + 7) * 4, 64))
            return 0;
    return 1;
}
/* Keep each previous image pinned while the next frame publishes, then independently reacquire. */
static int frame_export(Capture *test, const FVM_DesktopFrame *frame)
{
    size_t i, n;
    unsigned char bytes[32];
    FVM_RReply reply;
    FVM_DesktopResourceFrameInfo *info;
    if (fvm_desktop_resource_frame_capture(test->frame_adapter, frame) != FVM_R_OK)
        return 0;
    for (i = 0; i < 2; ++i)
    {
        info = test->frame_info + i;
        if (info->lease.id)
        {
            if (!frame_pixels(test, i, test->held_pixels[i]))
                return 0;
            n = frame_number(bytes, info->lease.id);
            n += frame_number(bytes + n, info->lease.generation);
            if (!test_9p_call(test->clients + i, test->dispatch, test->frame_target,
                              FVM_DESKTOP_RFRAME_RELEASE, bytes, n, &reply) ||
                reply.size)
                return 0;
        }
        if (!test_9p_call(test->clients + i, test->dispatch, test->frame_target,
                          FVM_DESKTOP_RFRAME_ACQUIRE, 0, 0, &reply) ||
            fvm_desktop_resource_frame_read(reply.data, reply.size, info) != FVM_R_OK ||
            !info->lease.id || info->sequence != test->count || info->width != 32 ||
            info->height != 32 || info->row_bytes != 128 ||
            info->byte_count != sizeof(test->copy) || info->format != frame->format ||
            info->reason != frame->reason || info->movie_ticks != frame->movie_ticks ||
            info->total_ticks != frame->total_ticks ||
            info->presentations != frame->presentations ||
            info->total_presentations != frame->total_presentations || info->flags ||
            info->dropped || strcmp(info->source, frame->source) ||
            !frame_pixels(test, i, test->copy))
            return 0;
        memcpy(test->held_pixels[i], test->copy, sizeof(test->copy));
    }
    return test->frame_info[0].lease.id != test->frame_info[1].lease.id;
}
#endif
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
#ifdef FVM_TEST_RESOURCE_9P
    if (!frame_export(test, frame))
        goto failed;
#endif
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
#ifdef FVM_TEST_RESOURCE_CONTROL
    if (!strcmp(test->scenario, "stop-active"))
        return test->received && state->total_ticks >= 2 ? FVM_DESKTOP_PLAYBACK_STOP
                                                         : FVM_DESKTOP_PLAYBACK_RESUME;
#endif
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
#ifdef FVM_TEST_RESOURCE_CONTROL
/* Establish one incoming resource-request schema; no VM/context/root pointer enters the wire. */
static int resource_setup(Capture *test)
{
    FVM_SBinding binding;
    unsigned char payload[128], wire[256];
    size_t size, encoded, consumed;
    int ok;
    memset(&binding, 0, sizeof(binding));
    binding.tag = 3;
    binding.version = FVM_RWIRE_VERSION;
    binding.type_size = (unsigned int)strlen(FVM_RWIRE_REQUEST_TYPE);
    memcpy(binding.type, FVM_RWIRE_REQUEST_TYPE, binding.type_size);
    ok = fvm_rdispatch_create(0, &test->dispatch) == FVM_R_OK &&
         fvm_desktop_resource_control_init(&test->adapter, test->dispatch, "/timeline/ctl") ==
             FVM_R_OK &&
         fvm_rdispatch_grant(test->dispatch, test->adapter.target, 7, FVM_R_CONTROL) == FVM_R_OK &&
         fvm_sbindings_init(&test->incoming, test->types, 1) == FVM_S_OK &&
         fvm_sbindings_payload_write(&binding, 1, payload, sizeof(payload), &size) == FVM_S_OK &&
         fvm_srecord_write(0, payload, size, wire, sizeof(wire), &encoded) == FVM_S_OK &&
         fvm_sbindings_apply(&test->incoming, wire, encoded, &consumed) == FVM_S_OK;
#ifdef FVM_TEST_RESOURCE_9P
    if (!ok)
        return 0;
    return fvm_desktop_resource_state_create(test->dispatch, "/timeline/state",
                                             &test->state_adapter,
                                             &test->state_target) == FVM_R_OK &&
           fvm_rdispatch_grant(test->dispatch, test->state_target, 7,
                               FVM_R_READ | FVM_R_SUBSCRIBE) == FVM_R_OK &&
           fvm_rdispatch_grant(test->dispatch, test->state_target, 8,
                               FVM_R_READ | FVM_R_SUBSCRIBE) == FVM_R_OK &&
           fvm_desktop_resource_frame_create(test->dispatch, "/display/frame", sizeof(test->copy),
                                             &test->frame_adapter,
                                             &test->frame_target) == FVM_R_OK &&
           fvm_rdispatch_grant(test->dispatch, test->frame_target, 7, FVM_R_READ) == FVM_R_OK &&
           fvm_rdispatch_grant(test->dispatch, test->frame_target, 8, FVM_R_READ) == FVM_R_OK &&
           test_9p_create_sized(test->clients, test->dispatch, 7, test->state_target, 256) &&
           test_9p_create_sized(test->clients + 1, test->dispatch, 8, test->state_target, 384);
#else
    return ok;
#endif
}
/* Preserve the existing real-pixel policy but marshal every command through CRC/schema/queue. */
static FVM_DesktopPlaybackCommand resource_notify(const FVM_DesktopPlaybackState *state, void *raw)
{
    Capture *test = (Capture *)raw;
    FVM_DesktopPlaybackCommand wanted = control_frame(state, raw), delivered;
    FVM_RRequest request;
#ifndef FVM_TEST_RESOURCE_9P
    FVM_RReply reply;
    unsigned char wire[128];
    size_t encoded, consumed;
#endif
#ifdef FVM_TEST_RESOURCE_9P
    {
        size_t i;
        FVM_REvent event;
        FVM_RResult status;
        FVM_DesktopResourceSnapshot snapshot;
        if (fvm_desktop_resource_state_observe(test->state_adapter, state) != FVM_R_OK)
            goto failed;
        for (i = 0; i < 2; ++i)
        {
            status = test_9p_event(test->clients + i, &event);
            if (status == FVM_R_PENDING)
                continue;
            if (status != FVM_R_OK || event.missed ||
                fvm_desktop_resource_state_read(event.data, event.size, &snapshot) != FVM_R_OK ||
                snapshot.phase != (unsigned long)state->phase ||
                snapshot.paused != (unsigned long)state->paused ||
                snapshot.step_pending != (unsigned long)state->step_pending ||
                snapshot.movie_ticks != state->movie_ticks ||
                snapshot.total_ticks != state->total_ticks ||
                snapshot.presentations != state->presentations || snapshot.flags ||
                strcmp(snapshot.source, state->source))
                goto failed;
            if (event.kind == FVM_DESKTOP_STATE_READY)
                ++test->epochs[i];
            if (event.kind == FVM_DESKTOP_STATE_TICK)
                ++test->ticks_seen[i];
            if (snapshot.epoch != test->epochs[i])
                goto failed;
            ++test->state_events[i];
        }
    }
#endif
    if (wanted != FVM_DESKTOP_PLAYBACK_KEEP)
    {
        memset(&request, 0, sizeof(request));
        request.session = 999; /* Deliberately not trusted wire identity. */
        request.id = ++test->requests;
        request.target = test->adapter.target;
        switch (wanted)
        {
        case FVM_DESKTOP_PLAYBACK_PAUSE:
            request.operation = FVM_DESKTOP_RESOURCE_PAUSE;
            break;
        case FVM_DESKTOP_PLAYBACK_RESUME:
            request.operation = FVM_DESKTOP_RESOURCE_RESUME;
            break;
        case FVM_DESKTOP_PLAYBACK_STEP:
            request.operation = FVM_DESKTOP_RESOURCE_STEP;
            break;
        case FVM_DESKTOP_PLAYBACK_STOP:
            request.operation = FVM_DESKTOP_RESOURCE_STOP;
            break;
        default:
            goto failed;
        }
#ifdef FVM_TEST_RESOURCE_9P
        if (!test_9p_command(test->clients, test->adapter.target, request.operation))
            goto failed;
#else
        if (fvm_rrequest_record_write(3, &request, wire, sizeof(wire), &encoded) != FVM_S_OK ||
            fvm_rdispatch_post_record(test->dispatch, 7, &test->incoming, wire, encoded,
                                      &consumed) != FVM_R_OK ||
            consumed != encoded)
            goto failed;
#endif
    }
    delivered = fvm_desktop_resource_control_notify(state, &test->adapter);
    if (delivered != wanted)
        goto failed;
#ifdef FVM_TEST_RESOURCE_9P
    if (wanted != FVM_DESKTOP_PLAYBACK_KEEP && !test_9p_reply(test->clients))
        goto failed;
#else
    if (wanted != FVM_DESKTOP_PLAYBACK_KEEP &&
        (fvm_rdispatch_take(test->dispatch, 7, test->requests, &reply) != FVM_R_OK ||
         reply.result != FVM_R_OK || reply.size))
        goto failed;
#endif
    return delivered;
failed:
    test->failed = 1;
    return FVM_DESKTOP_PLAYBACK_STOP;
}
/* Cancel/ack any failure-path leftovers before resource/dispatcher teardown and SDK shutdown. */
static void resource_cleanup(Capture *test)
{
    FVM_RReply reply;
    unsigned long id;
    if (!test->dispatch)
        return;
#ifdef FVM_TEST_RESOURCE_9P
    if (test->state_events[0])
    {
        FVM_DesktopResourceSnapshot snapshot;
        if (!test_9p_query(test->clients, test->dispatch, test->state_target, &reply) ||
            fvm_desktop_resource_state_read(reply.data, reply.size, &snapshot) != FVM_R_OK ||
            snapshot.epoch != test->epochs[0])
            test->failed = 1;
    }
    if (test->frame_info[0].lease.id)
    {
        FVM_DesktopResourceFrameInfo info;
        if (!frame_pixels(test, 0, test->held_pixels[0]) ||
            !frame_pixels(test, 1, test->held_pixels[1]) ||
            !test_9p_call(test->clients, test->dispatch, test->frame_target,
                          FVM_DESKTOP_RFRAME_INFO, 0, 0, &reply) ||
            fvm_desktop_resource_frame_read(reply.data, reply.size, &info) != FVM_R_OK ||
            info.lease.id || info.sequence != test->count)
            test->failed = 1;
    }
    if (!test_9p_destroy(test->clients))
        test->failed = 1;
    if (!test_9p_destroy(test->clients + 1))
        test->failed = 1;
    if (test->frame_adapter)
    {
        if (fvm_desktop_resource_frame_revoke(test->frame_adapter, 7) != FVM_R_OK)
            test->failed = 1;
        if (fvm_desktop_resource_frame_revoke(test->frame_adapter, 8) != FVM_R_OK)
            test->failed = 1;
        if (fvm_desktop_resource_frame_destroy(&test->frame_adapter) != FVM_R_OK)
            test->failed = 1;
    }
    if (test->state_adapter && fvm_desktop_resource_state_destroy(&test->state_adapter) != FVM_R_OK)
        test->failed = 1;
    if (test->state_events[0] != test->state_events[1] || test->epochs[0] != test->epochs[1] ||
        test->ticks_seen[0] != test->ticks_seen[1])
        test->failed = 1;
    if (test->state_events[0] &&
        test->epochs[0] != (unsigned long)(!strcmp(test->scenario, "navigation") ? 2 : 1))
        test->failed = 1;
#endif
    for (id = 1; id <= test->requests; ++id)
    {
        fvm_rdispatch_cancel(test->dispatch, 7, id);
        fvm_rdispatch_take(test->dispatch, 7, id, &reply);
    }
    if (test->adapter.struct_size &&
        fvm_rdispatch_unregister(test->dispatch, test->adapter.target) != FVM_R_OK)
        test->failed = 1;
    if (fvm_rdispatch_destroy(&test->dispatch) != FVM_R_OK)
        test->failed = 1;
}
#endif
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
#ifdef FVM_TEST_RESOURCE_CONTROL
    controlled = controlled || !strcmp(test.scenario, "stop-active");
#endif
    fvm_desktop_options_init(&options);
    options.mode = !strcmp(argv[2], "--jit") ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
    options.mute = 1;
    options.stats = 1;
    options.frames =
        (!strcmp(test.scenario, "fail-active") || !strcmp(test.scenario, "stop-active")) ? 100 : 2;
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
#ifdef FVM_TEST_RESOURCE_CONTROL
    if (!resource_setup(&test))
    {
        resource_cleanup(&test);
        return 4;
    }
    control.notify = resource_notify;
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
#ifdef FVM_TEST_RESOURCE_CONTROL
    else if (!strcmp(test.scenario, "stop-active"))
    {
        if (status || test.count < 2 || !test.cancelled || !test.requests)
            test.failed = 1;
    }
#endif
    else if (status || test.count != (!strcmp(test.scenario, "sequence") ? 4UL : 2UL))
        test.failed = 1;
#ifdef FVM_TEST_OFFSCREEN
    if (SDL_WasInit(0) != host_subsystems || SDL_PollEvent(&host_event) != 1 ||
        host_event.type != SDL_USEREVENT || host_event.user.code != 720)
        test.failed = 1;
    SDL_QuitSubSystem(SDL_INIT_TIMER | SDL_INIT_EVENTS);
#endif
#ifdef FVM_TEST_RESOURCE_CONTROL
    resource_cleanup(&test);
#endif
#ifdef FVM_TEST_RESOURCE_9P
    if ((!strcmp(test.scenario, "sequence") || !strcmp(test.scenario, "navigation")) &&
        test.ticks_seen[0] != test.last_tick)
        test.failed = 1;
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
