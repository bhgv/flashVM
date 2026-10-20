/* Exercise live owner callbacks without a console parser or SDL types in host code. */
#include "../common/player.h"
#ifdef FVM_DESKTOP_SOCKET_HTTP
#include "../common/socket_http.h"
#elif defined(FVM_DESKTOP_HTTP)
#include "../common/http.h"
#endif
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include "flashvm/stage.h"
#include "flashvm/execution_mode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Mutable host state contains scalars only: no VM pointer survives a notification. */
typedef struct ControlTest
{
    const char *scenario;
    unsigned long ready, ticks, paused_turns, refreshes;
    double paused_at;
    int received, cancelled; /* Owner monitor proves the body began before cancellation. */
    int failed, resumed;
} ControlTest;
/* Fail the fixture while still letting the real runner perform orderly cleanup. */
static FVM_DesktopPlaybackCommand failure(ControlTest *test, const char *why)
{
    fprintf(stderr, "control fixture: %s\n", why);
    test->failed = 1;
    return FVM_DESKTOP_PLAYBACK_STOP;
}
/* Read one current-root scalar without storing the managed owner. */
static int number(const FVM_DesktopPlaybackState *state, const char *key, double *out)
{
    FVM_Value value;
    if (fvm_object_get(state->context, state->root, key, &value) != FVM_OK ||
        value.type != FVM_VALUE_NUMBER)
        return 0;
    *out = value.number;
    return 1;
}
/* Observe worker bytes/disposition without entering VM code from the load monitor. */
static int load_notice(const FVM_DesktopLoadEvent *event, void *raw)
{
    ControlTest *test = (ControlTest *)raw;
    if (event->kind == FVM_DESKTOP_LOAD_VARIABLES && event->received)
        test->received = 1;
    if (event->kind == FVM_DESKTOP_LOAD_VARIABLES && event->phase == FVM_DESKTOP_LOAD_CANCELLED)
        test->cancelled = 1;
    return 0;
}

/* At every safe point, query the native graph and collect using the player's existing pins. */
static FVM_DesktopPlaybackCommand observe(const FVM_DesktopPlaybackState *state, void *raw)
{
    ControlTest *test = (ControlTest *)raw;
    FVM_MovieClipContentInfo content;
    FVM_Value value;
    double actions = 0, delivered = 0;
    if (!state || state->struct_size != sizeof(*state) || !state->context || !state->root ||
        !state->source || state->elapsed_ms < 0)
        return failure(test, "invalid borrowed snapshot");
    if (fvm_movieclip_content_info_init(&content) != FVM_OK)
        return failure(test, "snapshot initialization failed");
    if (fvm_movieclip_get_content_info(state->context, state->root, &content) != FVM_OK ||
        content.format != FVM_MOVIECLIP_CONTENT_SWF || content.swf_version != 8)
        return failure(test, "missing current source metadata");
    if (fvm_gc_collect(state->context, 0) != FVM_OK)
        return failure(test, "owner collection failed");
    if (state->elapsed_ms > 3000)
        return failure(test, "controller failed to make progress");
    test->ticks = state->total_ticks;
    test->refreshes = state->presentations;
    if (state->phase == FVM_DESKTOP_PLAYBACK_READY)
    {
        ++test->ready;
        fvm_value_number(&value, 0);
        if (fvm_object_set(state->context, state->root, "actions", &value) != FVM_OK ||
            fvm_object_set(state->context, state->root, "timerRuns", &value) != FVM_OK)
            return failure(test, "cannot seed fixture counters");
        if (!strcmp(test->scenario, "ready-stop"))
            return FVM_DESKTOP_PLAYBACK_STOP;
        if (!strcmp(test->scenario, "bad"))
            return (FVM_DesktopPlaybackCommand)999;
        return FVM_DESKTOP_PLAYBACK_STEP;
    }
    if (!number(state, "actions", &actions) || actions != (double)state->movie_ticks)
        return failure(test, "timeline advanced outside permitted ticks");
    if (state->phase == FVM_DESKTOP_PLAYBACK_TICK)
    {
        if (!strcmp(test->scenario, "cancel-navigation"))
            return FVM_DESKTOP_PLAYBACK_STOP;
        if (!strcmp(test->scenario, "navigation"))
            return state->total_ticks == 2 ? FVM_DESKTOP_PLAYBACK_STOP : FVM_DESKTOP_PLAYBACK_KEEP;
        if (!strcmp(test->scenario, "slow") || !strcmp(test->scenario, "audio"))
            return state->total_ticks == 3 ? FVM_DESKTOP_PLAYBACK_STOP : FVM_DESKTOP_PLAYBACK_STEP;
        if (state->total_ticks == 1)
        {
            test->paused_at = state->elapsed_ms;
            return FVM_DESKTOP_PLAYBACK_PAUSE;
        }
        if (state->total_ticks == 2)
        {
            test->resumed = 1;
            return FVM_DESKTOP_PLAYBACK_RESUME;
        }
        return FVM_DESKTOP_PLAYBACK_STOP;
    }
    if (state->paused && !state->step_pending && state->movie_ticks == 1)
    {
        ++test->paused_turns;
        if (!strcmp(test->scenario, "network"))
        {
            if (fvm_object_get(state->context, state->root, "answer", &value) != FVM_OK)
                return failure(test, "cannot read transferred variables");
            if (value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 5 &&
                !memcmp(value.string, "ready", 5))
                return FVM_DESKTOP_PLAYBACK_STOP;
            return FVM_DESKTOP_PLAYBACK_KEEP;
        }
        if (!strcmp(test->scenario, "cancel"))
            return test->received && state->elapsed_ms - test->paused_at >= 40
                       ? FVM_DESKTOP_PLAYBACK_STOP
                       : FVM_DESKTOP_PLAYBACK_KEEP;
        if (fvm_stage_request_update(state->context) != FVM_OK)
            return failure(test, "paused redraw failed");
        if (state->elapsed_ms - test->paused_at >= 40)
        {
            if (!number(state, "timerRuns", &delivered) || delivered < 1)
                return failure(test, "timers froze with timeline");
            return FVM_DESKTOP_PLAYBACK_STEP;
        }
    }
    return FVM_DESKTOP_PLAYBACK_KEEP;
}
/* Keep startup bytes borrowed and counted until the synchronous runner returns. */
static unsigned char *read_bytes(const char *path, size_t *size)
{
    FILE *file = fopen(path, "rb");
    long length;
    unsigned char *bytes = 0;
    if (!file)
        return 0;
    if (!fseek(file, 0, SEEK_END) && (length = ftell(file)) > 0 && !fseek(file, 0, SEEK_SET))
    {
        bytes = (unsigned char *)malloc((size_t)length);
        if (bytes && fread(bytes, 1, (size_t)length, file) == (size_t)length)
            *size = (size_t)length;
        else
        {
            free(bytes);
            bytes = 0;
        }
    }
    fclose(file);
    return bytes;
}
int main(int argc, char **argv)
{
    FVM_DesktopOptions options;
    FVM_DesktopSource source;
    FVM_DesktopLoader loader;
    FVM_DesktopBackgroundLoader background;
    FVM_DesktopLoadMonitor monitor;
    FVM_DesktopPlaybackControl control;
    ControlTest test;
    const char *error = 0;
    unsigned char *memory = 0;
    size_t size = 0;
    int status;
    if (argc < 4 || argc > 5)
        return 2;
    if (!strcmp(argv[3], "audio"))
    {
        FVM_DesktopBuildInfo info;
        if (fvm_desktop_build_info(&info) != FVM_OK)
            return 3;
        if (!info.sdl_audio_compiled)
            return 77;
    }
    memset(&test, 0, sizeof(test));
    test.scenario = argv[3];
    fvm_desktop_options_init(&options);
    fvm_desktop_loader_init(&loader);
    fvm_desktop_background_loader_init(&background);
    fvm_desktop_load_monitor_init(&monitor);
    monitor.notify = load_notice;
    monitor.userdata = &test;
    fvm_desktop_playback_control_init(&control);
    control.initially_paused = 1;
    control.notify = observe;
    control.userdata = &test;
    options.mode = !strcmp(argv[2], "--jit") ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
    options.frames = 500;
    options.fps = (!strcmp(argv[3], "slow") || !strcmp(argv[3], "audio")) ? 0.1 : 1000;
    options.mute = strcmp(argv[3], "audio") != 0;
    options.stats = 1;
    options.max_loads = 8;
    loader.max_loads = 8;
    background.max_loads = 8;
    memset(&source, 0, sizeof(source));
    source.location = argv[1];
    source.kind = strstr(argv[1], "://") ? FVM_DESKTOP_SOURCE_URL : FVM_DESKTOP_SOURCE_FILE;
    if (argc == 5)
    {
        memory = read_bytes(argv[4], &size);
        if (!memory)
            return 3;
        source.kind = FVM_DESKTOP_SOURCE_MEMORY;
        source.bytes = memory;
        source.size = size;
    }
#ifdef FVM_DESKTOP_SOCKET_HTTP
    background.transfer.fetch = fvm_desktop_socket_transfer_fetch;
#elif defined(FVM_DESKTOP_HTTP)
    background.transfer.fetch = fvm_desktop_http_transfer_fetch;
#endif
    if (strstr(source.location, "://"))
    {
        if (!background.transfer.fetch)
        {
            free(memory);
            return 77;
        }
        status = fvm_desktop_run_source_background_controlled(&options, &source, &background,
                                                              &monitor, &control, &error);
    }
    else
        status = fvm_desktop_run_source_controlled(&options, &source, &loader, &control, &error);
    free(memory);
    if (!strcmp(argv[3], "bad"))
        status = status == FVM_DESKTOP_EXIT_PLAYBACK ? 0 : 4;
    else if (!status)
    {
        unsigned long expected = (!strcmp(argv[3], "ready-stop")) ? 0
                                 : (!strcmp(argv[3], "cancel") || !strcmp(argv[3], "network") ||
                                    !strcmp(argv[3], "cancel-navigation"))
                                     ? 1
                                 : !strcmp(argv[3], "navigation") ? 2
                                                                  : 3;
        if (test.ticks != expected || (!strcmp(argv[3], "navigation") && test.ready != 2) ||
            (!strcmp(argv[3], "sequence") && (!test.paused_turns || !test.resumed)))
            test.failed = 1;
    }
    if (!strcmp(argv[3], "cancel") && !test.cancelled)
        test.failed = 1;
    printf("control ready=%lu ticks=%lu paused_turns=%lu presentations=%lu failed=%d\n", test.ready,
           test.ticks, test.paused_turns, test.refreshes, test.failed);
    if (fvm_desktop_shutdown() != FVM_OK)
        test.failed = 1;
    return status || test.failed ? 1 : 0;
}
