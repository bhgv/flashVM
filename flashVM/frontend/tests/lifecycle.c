/* Synthetic lifecycle/foreign-window events through a real SDL dummy window. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/sdl_window.c"


static void event(FVM_DesktopWindow *v, Uint32 type)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = type;
    if (!(SDL_PushEvent(&e) == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_events(v, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static int key(FVM_DesktopWindow *v, SDL_Keycode code, Uint32 window)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = SDL_KEYDOWN;
    e.key.keysym.sym = code;
    e.key.windowID = window;
    return handle_event(v, &e);
}


int main(void)
{
    FVM_DesktopWindow *v;
    SDL_Event e;
    Uint32 own, other;
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
    v = fvm_desktop_window_open("lifecycle", 2, 2);
    if (!(v))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    own = SDL_GetWindowID(v->window);
    other = own + 1;
    if (!(key(v, SDLK_LEFT, own)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(v->input.left && v->event_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event(v, SDL_APP_WILLENTERBACKGROUND);
    if (!(!fvm_desktop_window_active(v)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!v->input.left && !v->event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(key(v, SDLK_RIGHT, own)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(key(v, SDLK_ESCAPE, own)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&e, 0, sizeof(e));
    e.type = SDL_MOUSEBUTTONDOWN;
    e.button.button = SDL_BUTTON_LEFT;
    if (!(handle_event(v, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&e, 0, sizeof(e));
    e.type = SDL_FINGERDOWN;
    e.tfinger.touchId = 1;
    e.tfinger.fingerId = 1;
    if (!(handle_event(v, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!v->event_count && !v->input.buttons && !v->touch_active && !v->input.right))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event(v, SDL_APP_WILLENTERFOREGROUND);
    if (!(!fvm_desktop_window_active(v)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event(v, SDL_APP_DIDENTERFOREGROUND);
    if (!(fvm_desktop_window_active(v)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(key(v, SDLK_RIGHT, other)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(key(v, SDLK_ESCAPE, other)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&e, 0, sizeof(e));
    e.type = SDL_WINDOWEVENT;
    e.window.windowID = other;
    e.window.event = SDL_WINDOWEVENT_CLOSE;
    if (!(handle_event(v, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!v->event_count && !v->input.right))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#if SDL_VERSION_ATLEAST(2, 0, 12)
    memset(&e, 0, sizeof(e));
    e.type = SDL_FINGERDOWN;
    e.tfinger.windowID = other;
    e.tfinger.touchId = 1;
    e.tfinger.fingerId = 2;
    e.tfinger.x = 0.5f;
    e.tfinger.y = 0.5f;
    if (!(handle_event(v, &e) && !v->touch_active && !v->event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.tfinger.windowID = own;
    if (!(handle_event(v, &e) && v->touch_active && v->event_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.type = SDL_FINGERUP;
    e.tfinger.windowID = other;
    if (!(handle_event(v, &e) && v->touch_active && v->event_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.tfinger.windowID = own;
    if (!(handle_event(v, &e) && !v->touch_active && v->event_count == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v->event_count = v->event_head = 0;
#endif
    if (!(key(v, SDLK_RIGHT, own)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(v->event_count == 1 && v->input.right))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event(v, SDL_APP_DIDENTERBACKGROUND);
    if (!(!fvm_desktop_window_active(v) && !v->event_count && !v->input.right))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event(v, SDL_APP_DIDENTERFOREGROUND);
    if (!(!key(v, SDLK_ESCAPE, own)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&e, 0, sizeof(e));
    e.type = SDL_QUIT;
    if (!(!handle_event(v, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_status(v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_desktop_window_close(v);
    return 0;
}
