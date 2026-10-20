/* Synthetic lifecycle/foreign-window events through a real SDL dummy window. */
#include "../common/sdl_window.c"
#include <assert.h>
static void event(FVM_DesktopWindow *v, Uint32 type)
{
    SDL_Event e; memset(&e, 0, sizeof(e)); e.type = type;
    assert(SDL_PushEvent(&e) == 1);
    assert(fvm_desktop_window_events(v, 0));
}
static int key(FVM_DesktopWindow *v, SDL_Keycode code, Uint32 window)
{
    SDL_Event e; memset(&e, 0, sizeof(e)); e.type = SDL_KEYDOWN;
    e.key.keysym.sym = code; e.key.windowID = window;
    return handle_event(v, &e);
}
int main(void)
{
    FVM_DesktopWindow *v; SDL_Event e; Uint32 own, other;
    assert(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0);
    assert(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software"));
    v = fvm_desktop_window_open("lifecycle", 2, 2); assert(v);
    own = SDL_GetWindowID(v->window); other = own + 1;
    assert(key(v, SDLK_LEFT, own));
    assert(v->input.left && v->event_count == 1);
    event(v, SDL_APP_WILLENTERBACKGROUND);
    assert(!fvm_desktop_window_active(v));
    assert(!v->input.left && !v->event_count);
    assert(key(v, SDLK_RIGHT, own));
    assert(key(v, SDLK_ESCAPE, own));
    memset(&e, 0, sizeof(e)); e.type = SDL_MOUSEBUTTONDOWN;
    e.button.button = SDL_BUTTON_LEFT; assert(handle_event(v, &e));
    memset(&e, 0, sizeof(e)); e.type = SDL_FINGERDOWN;
    e.tfinger.touchId = 1; e.tfinger.fingerId = 1;
    assert(handle_event(v, &e));
    assert(!v->event_count && !v->input.buttons && !v->touch_active && !v->input.right);
    event(v, SDL_APP_WILLENTERFOREGROUND);
    assert(!fvm_desktop_window_active(v));
    event(v, SDL_APP_DIDENTERFOREGROUND);
    assert(fvm_desktop_window_active(v));
    assert(key(v, SDLK_RIGHT, other));
    assert(key(v, SDLK_ESCAPE, other));
    memset(&e, 0, sizeof(e)); e.type = SDL_WINDOWEVENT;
    e.window.windowID = other; e.window.event = SDL_WINDOWEVENT_CLOSE;
    assert(handle_event(v, &e));
    assert(!v->event_count && !v->input.right);
#if SDL_VERSION_ATLEAST(2,0,12)
    memset(&e,0,sizeof(e)); e.type=SDL_FINGERDOWN;
    e.tfinger.windowID=other; e.tfinger.touchId=1; e.tfinger.fingerId=2;
    e.tfinger.x=0.5f; e.tfinger.y=0.5f;
    assert(handle_event(v,&e) && !v->touch_active && !v->event_count);
    e.tfinger.windowID=own;
    assert(handle_event(v,&e) && v->touch_active && v->event_count==1);
    e.type=SDL_FINGERUP; e.tfinger.windowID=other;
    assert(handle_event(v,&e) && v->touch_active && v->event_count==1);
    e.tfinger.windowID=own;
    assert(handle_event(v,&e) && !v->touch_active && v->event_count==2);
    v->event_count=v->event_head=0;
#endif
    assert(key(v, SDLK_RIGHT, own));
    assert(v->event_count == 1 && v->input.right);
    event(v, SDL_APP_DIDENTERBACKGROUND);
    assert(!fvm_desktop_window_active(v) && !v->event_count && !v->input.right);
    event(v, SDL_APP_DIDENTERFOREGROUND);
    assert(!key(v, SDLK_ESCAPE, own));
    memset(&e, 0, sizeof(e)); e.type = SDL_QUIT;
    assert(!handle_event(v, &e));
    assert(fvm_desktop_window_status(v) == FVM_OK);
    fvm_desktop_window_close(v); return 0;
}
