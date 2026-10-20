/* Actual SDL FIFO and VM callbacks, including failure consumption and origin restoration. */
#include "../common/sdl_window.c"
#include "flashvm/standard.h"
#include <assert.h>
static unsigned calls;
static int fail;
static double deltas[4];
static FVM_Result receive(FVM_Context *c, FVM_Object *self, unsigned int argc,
                          const FVM_Value *args, FVM_Value *out, void *user)
{
    double x, y;
    (void)self; (void)user;
    assert(argc == 1 && args[0].type == FVM_VALUE_NUMBER && calls < 4);
    assert(fvm_input_get_pointer(c, &x, &y) == FVM_OK && x == 15 && y == 27);
    deltas[calls++] = args[0].number;
    fvm_value_undefined(out); return fail ? FVM_ERROR : FVM_OK;
}
static void wheel(FVM_DesktopWindow *v, double amount, Uint32 direction)
{
    SDL_Event e; memset(&e, 0, sizeof(e)); e.type = SDL_MOUSEWHEEL;
    e.wheel.y = (Sint32)amount; e.wheel.direction = direction;
#if SDL_VERSION_ATLEAST(2,0,18)
    e.wheel.preciseY = (float)amount;
#endif
    assert(handle_event(v, &e));
}
int main(void)
{
    FVM_DesktopWindow *v; FVM_Config config; FVM_Runtime *r; FVM_Context *c; FVM_Object *listener;
    SDL_Event e; unsigned i;
    assert(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0);
    assert(SDL_SetHint(SDL_HINT_RENDER_DRIVER,"software"));
    v = fvm_desktop_window_open("wheel", 32, 32); assert(v);
    assert(fvm_desktop_window_events(v, 0)); v->event_count = v->event_head = 0;
    v->input.x = 10; v->input.y = 20;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &r) == FVM_OK);
    assert(fvm_context_create(r, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
    assert(fvm_object_create(c, &listener) == FVM_OK);
    assert(fvm_object_define_method_rooted(c,listener,"onMouseWheel",receive,NULL,NULL)==FVM_OK);
    assert(fvm_input_add_mouse_listener(c,listener)==FVM_OK);
    wheel(v, 2, SDL_MOUSEWHEEL_NORMAL); wheel(v, 1, SDL_MOUSEWHEEL_FLIPPED);
    assert(v->event_count == 2 && !v->input.clicks);
    fail = 1;
    assert(fvm_desktop_window_pump_input(v,c,5,7) == FVM_ERROR && calls == 1 && v->event_count == 1);
    fail = 0;
    assert(fvm_desktop_window_pump_input(v,c,5,7) == FVM_OK && calls == 2 && !v->event_count);
    assert(deltas[0] == 6 && deltas[1] == -3);
    wheel(v,0,SDL_MOUSEWHEEL_NORMAL); assert(!v->event_count);
    memset(&e,0,sizeof(e)); e.type=SDL_MOUSEWHEEL; e.wheel.y=1;
#if SDL_VERSION_ATLEAST(2,0,18)
    e.wheel.preciseY=1;
#endif
    e.wheel.which=SDL_TOUCH_MOUSEID; assert(handle_event(v,&e) && !v->event_count);
    e.wheel.which=0; v->touch_active=1; assert(handle_event(v,&e) && !v->event_count);
    v->touch_active=0; e.wheel.windowID=SDL_GetWindowID(v->window)+1;
    assert(handle_event(v,&e) && !v->event_count);
    e.wheel.windowID=0; v->background=1; assert(handle_event(v,&e) && !v->event_count);
    v->background=0;
    for(i=0;i<=FVM_DESKTOP_PENDING_EVENTS;++i) wheel(v,1,SDL_MOUSEWHEEL_NORMAL);
    assert(v->event_count==FVM_DESKTOP_PENDING_EVENTS && fvm_desktop_window_status(v)==FVM_ERROR);
    fvm_context_destroy(c); fvm_runtime_destroy(r); fvm_desktop_window_close(v); return 0;
}
