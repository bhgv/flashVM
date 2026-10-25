/* Test the actual tutorial callbacks against SDL dummy-video and a real VM. */
#define main tutorial_main
#include <stdio.h>
#include <stdlib.h>
#include "../../examples/frontends/sdl/main.c"

#undef main


static void position(Demo *d, double x, double y)
{
    double a, b;
    if (!(fvm_input_get_pointer(d->context, &a, &b) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a == x && b == y))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    int previous;
    SDLHost h;
    Demo d;
    SDL_Event e;
    unsigned char pixels[DEMO_WIDTH * DEMO_HEIGHT * 4];
    memset(&h, 0, sizeof(h));
    memset(&e, 0, sizeof(e));
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    if (!(SDL_Init(SDL_INIT_VIDEO) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    h.window = SDL_CreateWindow("tutorial test", 0, 0, DEMO_WIDTH, DEMO_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (!(h.window))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    h.renderer = SDL_CreateRenderer(h.window, -1, 0);
    if (!(h.renderer))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_RenderSetLogicalSize(h.renderer, DEMO_WIDTH, DEMO_HEIGHT) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    h.texture =
        SDL_CreateTexture(h.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, DEMO_WIDTH, DEMO_HEIGHT);
    if (!(h.texture))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(demo_open(&d) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    h.cursor_applied = -1;
    if (!(sdl_cursor(&h, &d) == FVM_OK && SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_cursor_visible(d.context, 0, &previous) == FVM_OK && previous == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sdl_cursor(&h, &d) == FVM_OK && SDL_ShowCursor(SDL_QUERY) == SDL_DISABLE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sdl_cursor(&h, &d) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_cursor_visible(d.context, 1, &previous) == FVM_OK && previous == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sdl_cursor(&h, &d) == FVM_OK && SDL_ShowCursor(SDL_QUERY) == SDL_ENABLE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(demo_frame(&d, sdl_present, &h) == FVM_OK && d.frames == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_RenderCopy(h.renderer, h.texture, NULL, NULL) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(SDL_RenderReadPixels(h.renderer, NULL, SDL_PIXELFORMAT_RGBA32, pixels, DEMO_WIDTH * 4) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[(100 * DEMO_WIDTH + 160) * 4 + 1] == DEMO_RELEASED_GREEN))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.type = SDL_FINGERDOWN;
    e.tfinger.touchId = 10;
    e.tfinger.fingerId = 20;
    e.tfinger.x = .25f;
    e.tfinger.y = .5f;
    if (!(sdl_event(&h, &d, &e) && h.touching && d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 80, 100);
    e.tfinger.fingerId = 21;
    e.tfinger.x = .75f;
    if (!(sdl_event(&h, &d, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 80, 100);
    e.type = SDL_FINGERUP;
    if (!(sdl_event(&h, &d, &e) && h.touching && d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.type = SDL_MOUSEMOTION;
    e.motion.which = 0;
    e.motion.x = 300;
    e.motion.y = 150;
    if (!(sdl_event(&h, &d, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 80, 100);
    e.type = SDL_FINGERUP;
    e.tfinger.touchId = 10;
    e.tfinger.fingerId = 20;
    e.tfinger.x = .5f;
    e.tfinger.y = .25f;
    if (!(sdl_event(&h, &d, &e) && !h.touching && !d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 160, 50);
    e.type = SDL_MOUSEMOTION;
    e.motion.which = SDL_TOUCH_MOUSEID;
    e.motion.x = 1;
    if (!(sdl_event(&h, &d, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 160, 50);
    e.type = SDL_MOUSEBUTTONDOWN;
    e.button.which = 0;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = 30;
    e.button.y = 40;
    if (!(sdl_event(&h, &d, &e) && d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 30, 40);
    e.type = SDL_FINGERDOWN;
    e.tfinger.touchId = 10;
    e.tfinger.fingerId = 30;
    if (!(sdl_event(&h, &d, &e) && !h.touching))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 30, 40);
    e.type = SDL_WINDOWEVENT;
    e.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
    if (!(sdl_event(&h, &d, &e) && !h.touching && !d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&d, 30, 40);
    if (!(demo_frame(&d, sdl_present, &h) == FVM_OK && d.frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&e, 0, sizeof(e));
    e.type = SDL_MOUSEWHEEL;
    e.wheel.y = 2;
#if SDL_VERSION_ATLEAST(2, 0, 18)
    e.wheel.preciseY = 0.5f;
#endif
    if (!(sdl_event(&h, &d, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#if SDL_VERSION_ATLEAST(2, 0, 18)
    if (!(d.wheel_events == 1 && d.last_wheel_delta == 1.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    if (!(d.wheel_events == 1 && d.last_wheel_delta == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    position(&d, 30, 40);
    e.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
    if (!(sdl_event(&h, &d, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(d.wheel_events == 2 && d.last_wheel_delta < 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.wheel.which = SDL_TOUCH_MOUSEID;
    if (!(sdl_event(&h, &d, &e) && d.wheel_events == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e.wheel.which = 0;
    h.touching = 1;
    if (!(sdl_event(&h, &d, &e) && d.wheel_events == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    h.touching = 0;
    e.type = SDL_QUIT;
    if (!(!sdl_event(&h, &d, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    demo_close(&d);
    demo_close(&d);
    SDL_DestroyTexture(h.texture);
    SDL_DestroyRenderer(h.renderer);
    SDL_DestroyWindow(h.window);
    SDL_Quit();
    return 0;
}
