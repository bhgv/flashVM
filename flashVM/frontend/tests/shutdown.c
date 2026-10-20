/* Matching library cleanup must refuse to terminate an embedding application's live SDL owners. */
#define SDL_MAIN_HANDLED
#include "../common/player.h"
#include <SDL.h>
#include <assert.h>
/* Exercise native references and a live host window, then explicitly finish the shared session. */
int main(void)
{
#ifndef FVM_DESKTOP_SHARED
    SDL_Window *host;
    Uint32 id;
    SDL_SetMainReady();
    assert(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0);
    assert(SDL_InitSubSystem(SDL_INIT_TIMER | SDL_INIT_VIDEO) == 0);
    host = SDL_CreateWindow("host owner", 0, 0, 2, 2, SDL_WINDOW_HIDDEN);
    assert(host);
    id = SDL_GetWindowID(host);
    assert(fvm_desktop_shutdown() == FVM_ERROR);
    assert(SDL_GetWindowFromID(id) == host);
    assert((SDL_WasInit(SDL_INIT_TIMER | SDL_INIT_VIDEO) & (SDL_INIT_TIMER | SDL_INIT_VIDEO)) ==
           (SDL_INIT_TIMER | SDL_INIT_VIDEO));
    SDL_DestroyWindow(host);
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER);
#endif
    /* Shared hosts may own a private static SDL, so only that DLL can finish its instance. */
    assert(fvm_desktop_shutdown() == FVM_OK);
    assert(fvm_desktop_shutdown() == FVM_OK);
    return 0;
}
