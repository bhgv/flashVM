/* Actual SDL FIFO and VM callbacks, including failure consumption and origin restoration. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/sdl_window.c"
#include "flashvm/standard.h"

static unsigned calls;
static int fail;
static double deltas[4];


static FVM_Result
receive(FVM_Context *c, FVM_Object *self, unsigned int argc, const FVM_Value *args, FVM_Value *out, void *user)
{
    double x, y;
    (void)self;
    (void)user;
    if (!(argc == 1 && args[0].type == FVM_VALUE_NUMBER && calls < 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_get_pointer(c, &x, &y) == FVM_OK && x == 15 && y == 27))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    deltas[calls++] = args[0].number;
    fvm_value_undefined(out);
    return fail ? FVM_ERROR : FVM_OK;
}


static void wheel(FVM_DesktopWindow *v, double amount, Uint32 direction)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = SDL_MOUSEWHEEL;
    e.wheel.y = (Sint32)amount;
    e.wheel.direction = direction;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    e.wheel.preciseY = (float)amount;
#endif
#if SDL_VERSION_ATLEAST(2, 26, 0)
    e.wheel.mouseX = 10;
    e.wheel.mouseY = 20;
#endif
    if (!(handle_event(v, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    FVM_DesktopWindow *v;
    FVM_Config config;
    FVM_Runtime *r;
    FVM_Context *c;
    FVM_Object *listener;
    SDL_Event e;
    unsigned i;
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
    v = fvm_desktop_window_open("wheel", 32, 32);
    if (!(v))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_events(v, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v->event_count = v->event_head = 0;
    v->input.x = 10;
    v->input.y = 20;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &r) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(r, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, listener, "onMouseWheel", receive, NULL, NULL) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_add_mouse_listener(c, listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    wheel(v, 2, SDL_MOUSEWHEEL_NORMAL);
    wheel(v, 1, SDL_MOUSEWHEEL_FLIPPED);
    if (!(v->event_count == 2 && !v->input.clicks))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail = 1;
    if (!(fvm_desktop_window_pump_input(v, c, 5, 7) == FVM_ERROR && calls == 1 && v->event_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail = 0;
    if (!(fvm_desktop_window_pump_input(v, c, 5, 7) == FVM_OK && calls == 2 && !v->event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(deltas[0] == 6 && deltas[1] == -3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#if SDL_VERSION_ATLEAST(2, 0, 18)
    wheel(v, 0.5, SDL_MOUSEWHEEL_NORMAL);
    if (!(fvm_desktop_window_pump_input(v, c, 5, 7) == FVM_OK && calls == 3 && deltas[2] == 1.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
#if SDL_VERSION_ATLEAST(2, 26, 0)
    SDL_SetWindowSize(v->window, 64, 32);
    if (!(fvm_desktop_window_events(v, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&e, 0, sizeof(e));
    e.type = SDL_MOUSEWHEEL;
    e.wheel.y = 1;
    e.wheel.preciseY = 1;
    e.wheel.mouseX = 26;
    e.wheel.mouseY = 20;
    e.wheel.windowID = SDL_GetWindowID(v->window);
    if (!(SDL_PushEvent(&e) == 1 && fvm_desktop_window_events(v, 0) && v->event_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(v->pending_events[v->event_head].x == 10 && v->pending_events[v->event_head].y == 20))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_pump_input(v, c, 5, 7) == FVM_OK && calls == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    wheel(v, 0, SDL_MOUSEWHEEL_NORMAL);
    if (!(!v->event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&e, 0, sizeof(e));
    e.type = SDL_MOUSEWHEEL;
    e.wheel.y = 1;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    e.wheel.preciseY = 1;
#endif
    e.wheel.which = SDL_TOUCH_MOUSEID;
    if (!(handle_event(v, &e) && !v->event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.wheel.which = 0;
    v->touch_active = 1;
    if (!(handle_event(v, &e) && !v->event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v->touch_active = 0;
    e.wheel.windowID = SDL_GetWindowID(v->window) + 1;
    if (!(handle_event(v, &e) && !v->event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.wheel.windowID = 0;
    v->background = 1;
    if (!(handle_event(v, &e) && !v->event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v->background = 0;
    for (i = 0; i <= FVM_DESKTOP_PENDING_EVENTS; ++i)
    {
        wheel(v, 1, SDL_MOUSEWHEEL_NORMAL);
    }
    if (!(v->event_count == FVM_DESKTOP_PENDING_EVENTS && fvm_desktop_window_status(v) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(r);
    fvm_desktop_window_close(v);
    return 0;
}
