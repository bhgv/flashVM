/* Matching library cleanup must refuse to terminate an embedding application's live SDL owners. */
#define SDL_MAIN_HANDLED
#include <stdio.h>
#include <stdlib.h>
#include "../common/player.h"
#include <SDL.h>


/* Exercise native references and a live host window, then explicitly finish the shared session. */
int main(void)
{
#ifndef FVM_DESKTOP_SHARED
    SDL_Window *host;
    Uint32 id;
    SDL_SetMainReady();
    if (!(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_InitSubSystem(SDL_INIT_TIMER | SDL_INIT_VIDEO) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    host = SDL_CreateWindow("host owner", 0, 0, 2, 2, SDL_WINDOW_HIDDEN);
    if (!(host))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    id = SDL_GetWindowID(host);
    if (!(fvm_desktop_shutdown() == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_GetWindowFromID(id) == host))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!((SDL_WasInit(SDL_INIT_TIMER | SDL_INIT_VIDEO) & (SDL_INIT_TIMER | SDL_INIT_VIDEO)) ==
          (SDL_INIT_TIMER | SDL_INIT_VIDEO)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_DestroyWindow(host);
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER);
#endif
    /* Shared hosts may own a private static SDL, so only that DLL can finish its instance. */
    if (!(fvm_desktop_shutdown() == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_shutdown() == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
