/* Inject a real cross-thread SDL close after partial child bytes, then verify joined VM teardown.
 */
#define SDL_MAIN_HANDLED
#include "player.h"
#include <SDL.h>
#include <string.h>

#ifdef FVM_DESKTOP_SOCKET_HTTP
#include "socket_http.h"

#else
#include "http.h"

#endif
typedef struct Sink
{
    FVM_DesktopWrite write;
    void *userdata;
    int child;
} Sink;

static SDL_threadID owner;
static SDL_atomic_t injected;


static FVM_Result sink(const void *bytes, size_t size, void *userdata)
{
    Sink *state = (Sink *)userdata;
    SDL_Event event;
    if (state->child && size && SDL_AtomicCAS(&injected, 0, 1))
    {
        memset(&event, 0, sizeof(event));
        event.type = SDL_QUIT;
        if (SDL_PushEvent(&event) != 1)
        {
            return FVM_ERROR;
        }
    }
    return state->write(bytes, size, state->userdata);
}


static FVM_Result fetch(const char *reference,
                        const char *base,
                        FVM_DesktopWrite write,
                        void *write_userdata,
                        char *resolved,
                        size_t capacity,
                        const FVM_NetControl *control,
                        void *userdata)
{
    Sink state;
    (void)userdata;
    if (SDL_ThreadID() == owner)
    {
        return FVM_ERROR;
    }
    state.write = write;
    state.userdata = write_userdata;
    state.child = strstr(reference, "child.swf") != 0;
#ifdef FVM_DESKTOP_SOCKET_HTTP
    return fvm_desktop_socket_transfer_fetch(reference, base, sink, &state, resolved, capacity, control, 0);
#else
    return fvm_desktop_http_transfer_fetch(reference, base, sink, &state, resolved, capacity, control, 0);
#endif
}


int main(int argc, char **argv)
{
    FVM_DesktopOptions options;
    FVM_DesktopSource source;
    FVM_DesktopBackgroundLoader loader;
    int status;
    if (argc != 2 && argc != 3)
    {
        return 2;
    }
    owner = SDL_ThreadID();
    fvm_desktop_options_init(&options);
    fvm_desktop_background_loader_init(&loader);
    options.mode = FVM_EXECUTION_INTERPRETER;
    options.frames = 3;
    options.fps = 10;
    options.mute = 1;
    loader.max_loads = 8;
    loader.transfer.fetch = fetch;
    source.kind = FVM_DESKTOP_SOURCE_FILE;
    source.location = argv[1];
    source.bytes = 0;
    source.size = 0;
    status = argc == 3 ? fvm_desktop_run_source_async(&options, &source, &loader, 0)
                       : fvm_desktop_run_source_background(&options, &source, &loader, 0);
    if (!SDL_AtomicGet(&injected) || fvm_desktop_transfer_active() || fvm_desktop_shutdown() != FVM_OK)
    {
        return 3;
    }
    return status;
}
