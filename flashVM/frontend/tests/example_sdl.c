/* Test the actual tutorial callbacks against SDL dummy-video and a real VM. */
#define main tutorial_main
#include "../../examples/frontends/sdl/main.c"
#undef main
#include <assert.h>
static void position(Demo *d, double x, double y)
{
    double a, b;
    assert(fvm_input_get_pointer(d->context, &a, &b) == FVM_OK);
    assert(a == x && b == y);
}
int main(void)
{
    SDLHost h; Demo d; SDL_Event e; unsigned char pixels[DEMO_WIDTH * DEMO_HEIGHT * 4];
    memset(&h, 0, sizeof(h)); memset(&e, 0, sizeof(e));
    SDL_SetMainReady(); SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    assert(SDL_Init(SDL_INIT_VIDEO) == 0);
    h.window = SDL_CreateWindow("tutorial test", 0, 0, DEMO_WIDTH, DEMO_HEIGHT, SDL_WINDOW_RESIZABLE);
    assert(h.window); h.renderer = SDL_CreateRenderer(h.window, -1, 0); assert(h.renderer);
    assert(SDL_RenderSetLogicalSize(h.renderer, DEMO_WIDTH, DEMO_HEIGHT) == 0);
    h.texture = SDL_CreateTexture(h.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
                                  DEMO_WIDTH, DEMO_HEIGHT); assert(h.texture);
    assert(demo_open(&d) == FVM_OK);
    assert(demo_frame(&d, sdl_present, &h) == FVM_OK && d.frames == 1);
    assert(SDL_RenderCopy(h.renderer, h.texture, NULL, NULL) == 0);
    assert(SDL_RenderReadPixels(h.renderer, NULL, SDL_PIXELFORMAT_RGBA32, pixels, DEMO_WIDTH * 4) == 0);
    assert(pixels[(100 * DEMO_WIDTH + 160) * 4 + 1] == DEMO_RELEASED_GREEN);
    e.type = SDL_FINGERDOWN; e.tfinger.touchId = 10; e.tfinger.fingerId = 20;
    e.tfinger.x = .25f; e.tfinger.y = .5f;
    assert(sdl_event(&h, &d, &e) && h.touching && d.held); position(&d, 80, 100);
    e.tfinger.fingerId = 21; e.tfinger.x = .75f;
    assert(sdl_event(&h, &d, &e)); position(&d, 80, 100);
    e.type = SDL_FINGERUP; assert(sdl_event(&h, &d, &e) && h.touching && d.held);
    e.type = SDL_MOUSEMOTION; e.motion.which = 0; e.motion.x = 300; e.motion.y = 150;
    assert(sdl_event(&h, &d, &e)); position(&d, 80, 100);
    e.type = SDL_FINGERUP; e.tfinger.touchId = 10; e.tfinger.fingerId = 20;
    e.tfinger.x = .5f; e.tfinger.y = .25f;
    assert(sdl_event(&h, &d, &e) && !h.touching && !d.held); position(&d, 160, 50);
    e.type = SDL_MOUSEMOTION; e.motion.which = SDL_TOUCH_MOUSEID; e.motion.x = 1;
    assert(sdl_event(&h, &d, &e)); position(&d, 160, 50);
    e.type = SDL_MOUSEBUTTONDOWN; e.button.which = 0; e.button.button = SDL_BUTTON_LEFT;
    e.button.x = 30; e.button.y = 40;
    assert(sdl_event(&h, &d, &e) && d.held); position(&d, 30, 40);
    e.type = SDL_FINGERDOWN; e.tfinger.touchId = 10; e.tfinger.fingerId = 30;
    assert(sdl_event(&h, &d, &e) && !h.touching); position(&d, 30, 40);
    e.type = SDL_WINDOWEVENT; e.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
    assert(sdl_event(&h, &d, &e) && !h.touching && !d.held); position(&d, 30, 40);
    assert(demo_frame(&d, sdl_present, &h) == FVM_OK && d.frames == 2);
    memset(&e,0,sizeof(e));e.type=SDL_MOUSEWHEEL;e.wheel.y=2;
#if SDL_VERSION_ATLEAST(2,0,18)
    e.wheel.preciseY=0.5f;
#endif
    assert(sdl_event(&h,&d,&e));
#if SDL_VERSION_ATLEAST(2,0,18)
    assert(d.wheel_events==1 && d.last_wheel_delta==1.5);
#else
    assert(d.wheel_events==1 && d.last_wheel_delta==6);
#endif
    position(&d,30,40);
    e.wheel.direction=SDL_MOUSEWHEEL_FLIPPED;assert(sdl_event(&h,&d,&e));
    assert(d.wheel_events==2 && d.last_wheel_delta<0);
    e.wheel.which=SDL_TOUCH_MOUSEID;assert(sdl_event(&h,&d,&e) && d.wheel_events==2);
    e.wheel.which=0;h.touching=1;assert(sdl_event(&h,&d,&e) && d.wheel_events==2);h.touching=0;
    e.type = SDL_QUIT; assert(!sdl_event(&h, &d, &e));
    demo_close(&d); demo_close(&d);
    SDL_DestroyTexture(h.texture); SDL_DestroyRenderer(h.renderer); SDL_DestroyWindow(h.window); SDL_Quit();
    return 0;
}
