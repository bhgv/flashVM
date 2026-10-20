/* Real owner-thread VM with a separate native controller using copied mailbox values only. */
#define SDL_MAIN_HANDLED
#include "../common/player.h"
#ifdef FVM_DESKTOP_SOCKET_HTTP
#include "../common/socket_http.h"
#elif defined(FVM_DESKTOP_HTTP)
#include "../common/http.h"
#endif
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CONTROLLER_BUDGET_MS 4000U
/* Shared native state; only owner callbacks ever receive a managed context/root. */
typedef struct Host
{
    FVM_DesktopPlaybackMailbox *mailbox;
    const char *scenario;
    SDL_atomic_t done, failed, received, cancelled, delivered;
    Uint32 started;
    unsigned long stopped_ticks;
} Host;
/* Poll a coherent scalar copy with a bounded lifetime-aware wait. */
static int await_ticks(Host *host, unsigned long minimum, int paused,
                       FVM_DesktopPlaybackSnapshot *snapshot)
{
    while (!SDL_AtomicGet(&host->done))
    {
        if (fvm_desktop_playback_mailbox_poll(host->mailbox, snapshot) == FVM_OK &&
            snapshot->total_ticks >= minimum && (paused < 0 || snapshot->paused == paused))
            return 1;
        if ((Uint32)(SDL_GetTicks() - host->started) > CONTROLLER_BUDGET_MS)
            break;
        SDL_Delay(1);
    }
    return 0;
}
/* Reject unobserved command loss while keeping a real failure stoppable through normal cleanup. */
static int post(Host *host, FVM_DesktopPlaybackCommand command)
{
    if (fvm_desktop_playback_mailbox_post(host->mailbox, command) == FVM_OK)
        return 1;
    SDL_AtomicSet(&host->failed, 1);
    return 0;
}
/* This OS worker never calls any VM, GC, window or SDL event API. */
static int controller(void *raw)
{
    Host *host = (Host *)raw;
    FVM_DesktopPlaybackSnapshot snapshot, later;
    Uint32 held;
    int failure = 0;
    fvm_desktop_playback_snapshot_init(&snapshot);
    fvm_desktop_playback_snapshot_init(&later);
    if (!strcmp(host->scenario, "startup-stop"))
    {
        while (!SDL_AtomicGet(&host->received) && !SDL_AtomicGet(&host->done) &&
               (Uint32)(SDL_GetTicks() - host->started) < CONTROLLER_BUDGET_MS)
            SDL_Delay(1);
        if (!SDL_AtomicGet(&host->received))
            failure = 1;
    }
    else if (!strcmp(host->scenario, "invalid"))
    {
        while (!SDL_AtomicGet(&host->done) &&
               (Uint32)(SDL_GetTicks() - host->started) < CONTROLLER_BUDGET_MS)
            SDL_Delay(1);
        if (!SDL_AtomicGet(&host->done))
            failure = 1;
    }
    else
    {
        if (!await_ticks(host, 0, -1, &snapshot))
            failure = 1;
        if (!failure && strcmp(host->scenario, "slow-stop"))
            post(host, FVM_DESKTOP_PLAYBACK_STEP);
        if (!failure && !strcmp(host->scenario, "runtime-error"))
        {
            while (!SDL_AtomicGet(&host->done) &&
                   (Uint32)(SDL_GetTicks() - host->started) < CONTROLLER_BUDGET_MS)
                SDL_Delay(1);
            if (!SDL_AtomicGet(&host->done))
                failure = 1;
        }
        else if (!failure && (!strcmp(host->scenario, "child-stop") ||
                              !strcmp(host->scenario, "child-wait-stop")))
        {
            while (!SDL_AtomicGet(&host->received) && !SDL_AtomicGet(&host->done) &&
                   (Uint32)(SDL_GetTicks() - host->started) < CONTROLLER_BUDGET_MS)
                SDL_Delay(1);
            if (!SDL_AtomicGet(&host->received))
                failure = 1;
        }
        else if (!failure)
        {
            if (!await_ticks(host, 1, -1, &snapshot))
                failure = 1;
            if (!failure && !strcmp(host->scenario, "sequence"))
            {
                held = SDL_GetTicks();
                while ((Uint32)(SDL_GetTicks() - held) < 40)
                {
                    if (fvm_desktop_playback_mailbox_poll(host->mailbox, &later) != FVM_OK ||
                        later.total_ticks != 1 || !later.paused)
                        failure = 1;
                    SDL_Delay(1);
                }
                if (!failure)
                    post(host, FVM_DESKTOP_PLAYBACK_STEP);
                if (!failure && !await_ticks(host, 2, -1, &snapshot))
                    failure = 1;
                if (!failure)
                    post(host, FVM_DESKTOP_PLAYBACK_RESUME);
                if (!failure && !await_ticks(host, 4, -1, &snapshot))
                    failure = 1;
                if (!failure)
                    post(host, FVM_DESKTOP_PLAYBACK_PAUSE);
                if (!failure && !await_ticks(host, 4, 1, &snapshot))
                    failure = 1;
                host->stopped_ticks = snapshot.total_ticks;
            }
            if (!failure && !strcmp(host->scenario, "network-pause"))
            {
                while (!SDL_AtomicGet(&host->delivered) && !SDL_AtomicGet(&host->done) &&
                       (Uint32)(SDL_GetTicks() - host->started) < CONTROLLER_BUDGET_MS)
                    SDL_Delay(1);
                if (!SDL_AtomicGet(&host->delivered) ||
                    fvm_desktop_playback_mailbox_poll(host->mailbox, &snapshot) != FVM_OK ||
                    snapshot.total_ticks != 1 || !snapshot.paused)
                    failure = 1;
            }
        }
    }
    if (failure)
        SDL_AtomicSet(&host->failed, 1);
    if (!SDL_AtomicGet(&host->done))
        post(host, FVM_DESKTOP_PLAYBACK_STOP);
    return 0;
}
/* [GC:] Only the owner queries/collects live VM objects; none escape into the native worker. */
static FVM_DesktopPlaybackCommand observe(const FVM_DesktopPlaybackState *state, void *raw)
{
    Host *host = (Host *)raw;
    FVM_Value value;
    if (fvm_gc_collect(state->context, 0) != FVM_OK)
    {
        SDL_AtomicSet(&host->failed, 1);
        return FVM_DESKTOP_PLAYBACK_STOP;
    }
    if (!strcmp(host->scenario, "network-pause") &&
        fvm_object_get(state->context, state->root, "answer", &value) == FVM_OK &&
        value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 5 &&
        !memcmp(value.string, "ready", 5))
        SDL_AtomicSet(&host->delivered, 1);
    return fvm_desktop_playback_mailbox_notify(state, host->mailbox);
}
/* Forward the exact same stop latch during startup/async/blocking resource waits. */
static int loading(const FVM_DesktopLoadEvent *event, void *raw)
{
    Host *host = (Host *)raw;
    unsigned int expected_kind = !strcmp(host->scenario, "startup-stop")
                                     ? FVM_DESKTOP_LOAD_ROOT : FVM_DESKTOP_LOAD_VARIABLES;
    if (event->kind == expected_kind && event->received)
        SDL_AtomicSet(&host->received, 1);
    if (event->kind == expected_kind && event->phase == FVM_DESKTOP_LOAD_CANCELLED)
        SDL_AtomicSet(&host->cancelled, 1);
    return fvm_desktop_playback_mailbox_load_notify(event, host->mailbox);
}
/* Count immutable input bytes; the caller keeps the resulting allocation through runner return. */
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
/* Main/UI thread owns synchronous playback; join the controller before disposing its mailbox. */
int main(int argc, char **argv)
{
    Host host;
    FVM_DesktopOptions options;
    FVM_DesktopSource source;
    FVM_DesktopPlaybackControl control;
    FVM_DesktopBackgroundLoader background;
    FVM_DesktopLoadMonitor monitor;
    FVM_DesktopPlaybackSnapshot snapshot;
    SDL_Thread *thread = 0;
    unsigned char *bytes = 0;
    size_t size = 0;
    const char *error = 0;
    int status, joined = 0, expected_error, network;
    if (argc < 4 || argc > 5)
        return 2;
    memset(&host, 0, sizeof(host));
    host.scenario = argv[3];
    host.started = SDL_GetTicks();
    network = strstr(argv[1], "://") != 0;
    fvm_desktop_options_init(&options);
    options.mode = !strcmp(argv[2], "--jit") ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
    options.mute = 1;
    options.stats = 1;
    options.frames = 1000;
    options.fps = !strcmp(host.scenario, "slow-stop") ? 0.1 : 40;
    fvm_desktop_playback_control_init(&control);
    control.initially_paused = strcmp(host.scenario, "slow-stop") != 0;
    control.notify = observe;
    control.userdata = &host;
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
    monitor.timeout_ms = 2000;
    monitor.notify = loading;
    monitor.userdata = &host;
    monitor.asynchronous = strcmp(host.scenario, "child-wait-stop") != 0;
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
    if (fvm_desktop_playback_mailbox_create(&host.mailbox) != FVM_OK)
    {
        free(bytes);
        return 3;
    }
    if (!strcmp(host.scenario, "pre-stop"))
        post(&host, FVM_DESKTOP_PLAYBACK_STOP);
    else
    {
        thread = SDL_CreateThread(controller, "player controller", &host);
        if (!thread)
        {
            fvm_desktop_playback_mailbox_release(host.mailbox);
            free(bytes);
            return 3;
        }
    }
    if (network)
        status = fvm_desktop_run_source_background_controlled(&options, &source, &background,
                                                              &monitor, &control, &error);
    else
        status = fvm_desktop_run_source_controlled(&options, &source, 0, &control, &error);
    SDL_AtomicSet(&host.done, 1);
    if (thread)
        SDL_WaitThread(thread, &joined);
    free(bytes);
    expected_error = !strcmp(host.scenario, "runtime-error") || !strcmp(host.scenario, "invalid");
    if ((expected_error ? status != 1 : status != 0) || joined)
        SDL_AtomicSet(&host.failed, 1);
    fvm_desktop_playback_snapshot_init(&snapshot);
    if (!strcmp(host.scenario, "pre-stop") || !strcmp(host.scenario, "startup-stop") ||
        !strcmp(host.scenario, "invalid"))
    {
        if (fvm_desktop_playback_mailbox_poll(host.mailbox, &snapshot) != FVM_NOT_FOUND)
            SDL_AtomicSet(&host.failed, 1);
    }
    else if (fvm_desktop_playback_mailbox_poll(host.mailbox, &snapshot) != FVM_OK)
        SDL_AtomicSet(&host.failed, 1);
    if (!strcmp(host.scenario, "sequence") &&
        (snapshot.total_ticks < 4 || snapshot.total_ticks != host.stopped_ticks))
        SDL_AtomicSet(&host.failed, 1);
    if (!strcmp(host.scenario, "slow-stop") && snapshot.total_ticks != 1)
        SDL_AtomicSet(&host.failed, 1);
    if (strstr(host.scenario, "stop") && network && strcmp(host.scenario, "pre-stop") &&
        !SDL_AtomicGet(&host.cancelled))
        SDL_AtomicSet(&host.failed, 1);
    if (fvm_desktop_shutdown() != FVM_ERROR)
        SDL_AtomicSet(&host.failed, 1);
    fvm_desktop_playback_mailbox_release(host.mailbox);
    if (fvm_desktop_shutdown() != FVM_OK || fvm_desktop_playback_mailbox_active())
        SDL_AtomicSet(&host.failed, 1);
    printf("mailbox ticks=%lu received=%d cancelled=%d failed=%d\n", snapshot.total_ticks,
           SDL_AtomicGet(&host.received), SDL_AtomicGet(&host.cancelled),
           SDL_AtomicGet(&host.failed));
    return SDL_AtomicGet(&host.failed) ? 1 : 0;
}
