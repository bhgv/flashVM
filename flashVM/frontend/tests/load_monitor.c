/* Real provider/owner separation, complete notice sequences and cancellation before import. */
#define SDL_MAIN_HANDLED
#include "player.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef FVM_DESKTOP_SOCKET_HTTP
#include "socket_http.h"

#else
#include "http.h"

#endif
typedef struct State
{
    SDL_threadID owner;
    SDL_atomic_t calls;
    unsigned int mode, begun, terminal, waits, stopped;
    unsigned int phase[16], kind[16];
    size_t received[16];
} State;


static FVM_Result fetch(const char *reference,
                        const char *base,
                        FVM_DesktopWrite write,
                        void *sink,
                        char *resolved,
                        size_t capacity,
                        const FVM_NetControl *control,
                        void *userdata)
{
    State *state = (State *)userdata;
    if (!(SDL_ThreadID() != state->owner))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_AtomicAdd(&state->calls, 1);
#ifdef FVM_DESKTOP_SOCKET_HTTP
    return fvm_desktop_socket_transfer_fetch(reference, base, write, sink, resolved, capacity, control, 0);
#else
    return fvm_desktop_http_transfer_fetch(reference, base, write, sink, resolved, capacity, control, 0);
#endif
}


static int notify(const FVM_DesktopLoadEvent *event, void *userdata)
{
    State *s = (State *)userdata;
    unsigned long id = event->id;
    int stop;
    if (!(SDL_ThreadID() == s->owner && id && id < 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event->reference && *event->reference))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(event->kind >= FVM_DESKTOP_LOAD_ROOT && event->kind <= FVM_DESKTOP_LOAD_VARIABLES))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (event->phase == FVM_DESKTOP_LOAD_BEGIN)
    {
        if (!(id == ++s->begun && !s->phase[id] && !event->received))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        s->kind[id] = event->kind;
        if (event->kind == FVM_DESKTOP_LOAD_ROOT)
        {
            if (!(!SDL_WasInit(SDL_INIT_VIDEO)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        if (!(s->phase[id] == FVM_DESKTOP_LOAD_BEGIN || s->phase[id] == FVM_DESKTOP_LOAD_WAIT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(s->kind[id] == event->kind && event->received >= s->received[id]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!((event->phase == FVM_DESKTOP_LOAD_FETCHED) == (event->location != 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (event->phase == FVM_DESKTOP_LOAD_BEGIN || event->phase == FVM_DESKTOP_LOAD_WAIT)
    {
        if (!(event->result == FVM_NOT_FOUND))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!((event->phase == FVM_DESKTOP_LOAD_FETCHED) == (event->result == FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ++s->terminal;
        printf("terminal id=%lu kind=%u phase=%u received=%lu\n",
               id,
               event->kind,
               event->phase,
               (unsigned long)event->received);
    }
    if (event->phase == FVM_DESKTOP_LOAD_WAIT)
    {
        ++s->waits;
    }
    s->phase[id] = event->phase;
    s->received[id] = event->received;
    stop = ((s->mode == 1 || s->mode == 17) && event->phase == FVM_DESKTOP_LOAD_BEGIN) ||
           (s->mode == 2 && event->phase == FVM_DESKTOP_LOAD_WAIT && event->received) ||
           (s->mode == 4 && event->kind == FVM_DESKTOP_LOAD_CHILD && event->phase == FVM_DESKTOP_LOAD_WAIT &&
            event->received) ||
           (s->mode == 9 && event->kind == FVM_DESKTOP_LOAD_CHILD && event->phase == FVM_DESKTOP_LOAD_BEGIN);
    if (stop)
    {
        s->stopped = 1;
    }
    /* Terminal return deliberately requests stop too: it must be ignored. */
    return stop || event->phase == FVM_DESKTOP_LOAD_FETCHED;
}


int main(int argc, char **argv)
{
    State state;
    FVM_DesktopOptions options;
    FVM_DesktopSource source;
    FVM_DesktopBackgroundLoader loader;
    FVM_DesktopLoadMonitor monitor;
    const char *error = "sentinel";
    int status;
    const char *files[2];
    unsigned char memory[4096];
    if (argc != 4)
    {
        return 2;
    }
    if (!fvm_desktop_background_supported())
    {
        return 77;
    }
    memset(&state, 0, sizeof(state));
    state.owner = SDL_ThreadID();
    state.mode = (unsigned int)atoi(argv[2]);
    if (!(!fvm_desktop_load_monitor_init(0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_load_monitor_init(&monitor)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(monitor.asynchronous == 1 && !monitor.timeout_ms && !monitor.notify && !monitor.userdata))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_desktop_options_init(&options);
    options.frames = state.mode == 6 ? 2 : 700;
    options.fps = 1000;
    options.mute = 1;
    options.mode = FVM_EXECUTION_INTERPRETER;
    fvm_desktop_background_loader_init(&loader);
    loader.max_loads = 8;
    loader.transfer.fetch = fetch;
    loader.transfer.userdata = &state;
    source.kind = FVM_DESKTOP_SOURCE_URL;
    source.location = argv[1];
    source.bytes = 0;
    source.size = 0;
    monitor.notify = notify;
    monitor.userdata = &state;
    if (state.mode == 3 || state.mode == 5)
    {
        monitor.timeout_ms = 80;
    }
    if (state.mode == 8)
    {
        monitor.asynchronous = 0;
    }
    if (state.mode == 10)
    {
        monitor.asynchronous = 2;
    }
    if (state.mode == 13)
    {
        monitor.timeout_ms = FVM_DESKTOP_TRANSFER_MAX_TIMEOUT_MS + 1UL;
    }
    if (state.mode == 14)
    {
        source.kind = 0;
    }
    if (state.mode == 11)
    {
        FILE *file = fopen(argv[3], "rb");
        if (!(file))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        source.size = fread(memory, 1, sizeof(memory), file);
        if (!(source.size && source.size < sizeof(memory) && !ferror(file)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fclose(file);
        source.kind = FVM_DESKTOP_SOURCE_MEMORY;
        source.bytes = memory;
    }
    if (state.mode == 16 || state.mode == 17)
    {
        files[0] = files[1] = argv[1];
        options.files = files;
        options.file_count = state.mode == 17 ? 2 : 1;
        options.max_loads = 8;
        status = fvm_desktop_run_monitored(&options, &monitor, &error);
    }
    else
    {
        status = fvm_desktop_run_source_monitored(&options, &source, &loader, &monitor, &error);
    }
    if (!(!fvm_desktop_transfer_active()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_shutdown() == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(state.begun == state.terminal))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (state.mode == 10 || state.mode == 13 || state.mode == 14)
    {
        if (!(status == 2 && error && !state.begun && !SDL_AtomicGet(&state.calls)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (state.mode == 3 || state.mode == 5)
    {
        unsigned int id = state.mode == 3 ? 1 : 2;
        if (!(status == 1 && error && state.phase[id] == FVM_DESKTOP_LOAD_TIMED_OUT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(state.received[id] > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (state.mode == 12)
    {
        if (!(status == 1 && error && state.phase[2] == FVM_DESKTOP_LOAD_FETCHED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (state.mode == 7)
    {
        if (!(status == 1 && error && state.phase[2] == FVM_DESKTOP_LOAD_FAILED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(status == 0 && !error))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (state.mode == 0 || state.mode == 8 || state.mode == 16)
        {
            if (!(state.begun == 4))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (state.mode == 11)
        {
            if (!(state.begun == 3 && state.kind[1] == FVM_DESKTOP_LOAD_CHILD))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (state.mode == 15)
        {
            if (!(state.begun == 2 && state.kind[2] == FVM_DESKTOP_LOAD_ROOT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (state.mode != 6)
        {
            if (!(state.stopped))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (state.mode == 1 || state.mode == 17)
        {
            if (!(!SDL_AtomicGet(&state.calls)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (state.mode == 4 || state.mode == 6 || state.mode == 9)
        {
            if (!(state.phase[2] == FVM_DESKTOP_LOAD_CANCELLED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    printf("summary begun=%u terminal=%u calls=%d waits=%u status=%d\n",
           state.begun,
           state.terminal,
           SDL_AtomicGet(&state.calls),
           state.waits,
           status);
    return 0;
}
