/* SDL subsystem ownership survives player close and failed native initialization. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/sdl_window.c"


/* Exercise references acquired before and after the player's private references. */
int main(void)
{
    FVM_DesktopWindow *first, *second;
    SDL_Window *host;
    Uint32 host_id;
    unsigned int i;
    const Uint32 unrelated = SDL_INIT_TIMER | SDL_INIT_EVENTS;
    SDL_SetMainReady();
    if (!(SDL_SetHintWithPriority(FVM_DESKTOP_TOUCH_MOUSE_HINT, "1", SDL_HINT_NORMAL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_SetHintWithPriority(FVM_DESKTOP_MOUSE_TOUCH_HINT, "1", SDL_HINT_NORMAL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_InitSubSystem(unrelated) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_setenv("SDL_VIDEODRIVER", "flashvm-invalid-driver", 1) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!fvm_desktop_window_open("must fail", 2, 2)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!((SDL_WasInit(unrelated) & unrelated) == unrelated))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!SDL_WasInit(SDL_INIT_VIDEO)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(SDL_GetHint(FVM_DESKTOP_TOUCH_MOUSE_HINT), "1")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!strcmp(SDL_GetHint(FVM_DESKTOP_MOUSE_TOUCH_HINT), "1")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* End this host session explicitly. SDL 2.30 can retain its own EVENTS
     * dependency after failed VIDEO startup; the player must not guess its refcount. */
    SDL_Quit();
    if (!(SDL_InitSubSystem(unrelated) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_InitSubSystem(SDL_INIT_VIDEO) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_SetHintWithPriority(FVM_DESKTOP_TOUCH_MOUSE_HINT, "1", SDL_HINT_NORMAL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_SetHintWithPriority(FVM_DESKTOP_MOUSE_TOUCH_HINT, "1", SDL_HINT_NORMAL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    host = SDL_CreateWindow("embedding host", 0, 0, 2, 2, SDL_WINDOW_HIDDEN);
    if (!(host))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    host_id = SDL_GetWindowID(host);
    if (!(host_id))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 3; ++i)
    {
        first = fvm_desktop_window_open("first", 2, 2);
        second = fvm_desktop_window_open("second", 2, 2);
        if (!(first && second))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!strcmp(SDL_GetHint(FVM_DESKTOP_TOUCH_MOUSE_HINT), "1")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!strcmp(SDL_GetHint(FVM_DESKTOP_MOUSE_TOUCH_HINT), "1")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_desktop_window_close(first);
        if (!(SDL_GetWindowFromID(host_id) == host))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(SDL_GetWindowID(second->window)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!((SDL_WasInit(unrelated | SDL_INIT_VIDEO) & (unrelated | SDL_INIT_VIDEO)) == (unrelated | SDL_INIT_VIDEO)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_desktop_window_close(second);
        if (!(SDL_GetWindowFromID(host_id) == host))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_desktop_window_close(0);
    SDL_DestroyWindow(host);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    if (!(!SDL_WasInit(SDL_INIT_VIDEO)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!((SDL_WasInit(unrelated) & unrelated) == unrelated))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* A lower-priority write must still fail: opening did not replace hint priority. */
    if (!(!SDL_SetHintWithPriority(FVM_DESKTOP_TOUCH_MOUSE_HINT, "0", SDL_HINT_DEFAULT)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!SDL_SetHintWithPriority(FVM_DESKTOP_MOUSE_TOUCH_HINT, "0", SDL_HINT_DEFAULT)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_QuitSubSystem(unrelated);
    if (!(SDL_WasInit(0) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SDL_Quit();
    return 0;
}
