/* SDL subsystem ownership survives player close and failed native initialization. */
#include "../common/sdl_window.c"
#include <assert.h>

/* Exercise references acquired before and after the player's private references. */
int main(void)
{
    FVM_DesktopWindow *first, *second;
    SDL_Window *host;
    Uint32 host_id;
    unsigned int i;
    const Uint32 unrelated = SDL_INIT_TIMER | SDL_INIT_EVENTS;
    SDL_SetMainReady();
    assert(SDL_SetHintWithPriority(FVM_DESKTOP_TOUCH_MOUSE_HINT, "1", SDL_HINT_NORMAL));
    assert(SDL_SetHintWithPriority(FVM_DESKTOP_MOUSE_TOUCH_HINT, "1", SDL_HINT_NORMAL));
    assert(SDL_InitSubSystem(unrelated) == 0);
    assert(SDL_setenv("SDL_VIDEODRIVER", "flashvm-invalid-driver", 1) == 0);
    assert(!fvm_desktop_window_open("must fail", 2, 2));
    assert((SDL_WasInit(unrelated) & unrelated) == unrelated);
    assert(!SDL_WasInit(SDL_INIT_VIDEO));
    assert(!strcmp(SDL_GetHint(FVM_DESKTOP_TOUCH_MOUSE_HINT), "1"));
    assert(!strcmp(SDL_GetHint(FVM_DESKTOP_MOUSE_TOUCH_HINT), "1"));
    /* End this host session explicitly. SDL 2.30 can retain its own EVENTS
     * dependency after failed VIDEO startup; the player must not guess its refcount. */
    SDL_Quit();
    assert(SDL_InitSubSystem(unrelated) == 0);
    assert(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0);
    assert(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software"));
    assert(SDL_InitSubSystem(SDL_INIT_VIDEO) == 0);
    assert(SDL_SetHintWithPriority(FVM_DESKTOP_TOUCH_MOUSE_HINT, "1", SDL_HINT_NORMAL));
    assert(SDL_SetHintWithPriority(FVM_DESKTOP_MOUSE_TOUCH_HINT, "1", SDL_HINT_NORMAL));
    host = SDL_CreateWindow("embedding host", 0, 0, 2, 2, SDL_WINDOW_HIDDEN);
    assert(host);
    host_id = SDL_GetWindowID(host);
    assert(host_id);
    for (i = 0; i < 3; ++i)
    {
        first = fvm_desktop_window_open("first", 2, 2);
        second = fvm_desktop_window_open("second", 2, 2);
        assert(first && second);
        assert(!strcmp(SDL_GetHint(FVM_DESKTOP_TOUCH_MOUSE_HINT), "1"));
        assert(!strcmp(SDL_GetHint(FVM_DESKTOP_MOUSE_TOUCH_HINT), "1"));
        fvm_desktop_window_close(first);
        assert(SDL_GetWindowFromID(host_id) == host);
        assert(SDL_GetWindowID(second->window));
        assert((SDL_WasInit(unrelated | SDL_INIT_VIDEO) & (unrelated | SDL_INIT_VIDEO)) ==
               (unrelated | SDL_INIT_VIDEO));
        fvm_desktop_window_close(second);
        assert(SDL_GetWindowFromID(host_id) == host);
    }
    fvm_desktop_window_close(0);
    SDL_DestroyWindow(host);
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    assert(!SDL_WasInit(SDL_INIT_VIDEO));
    assert((SDL_WasInit(unrelated) & unrelated) == unrelated);
    /* A lower-priority write must still fail: opening did not replace hint priority. */
    assert(!SDL_SetHintWithPriority(FVM_DESKTOP_TOUCH_MOUSE_HINT, "0", SDL_HINT_DEFAULT));
    assert(!SDL_SetHintWithPriority(FVM_DESKTOP_MOUSE_TOUCH_HINT, "0", SDL_HINT_DEFAULT));
    SDL_QuitSubSystem(unrelated);
    assert(SDL_WasInit(0) == 0);
    SDL_Quit();
    return 0;
}
