/* Timer-only adapter contract: unavailable video, bounded rasters and untouched host events. */
#define SDL_MAIN_HANDLED
#include "../common/window.h"
#include <SDL.h>
#include <stdio.h>
#include <string.h>

#define CHECK(x)                                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(x))                                                                                                      \
        {                                                                                                              \
            fprintf(stderr, "offscreen line %d: %s (%s)\n", __LINE__, #x, SDL_GetError());                             \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)


/* Assert independently owned SDL references survive failed and successful sessions. */
int main(void)
{
    FVM_DesktopWindow *view;
    FVM_RasterSurface surface;
    FVM_DesktopInput input;
    SDL_Event event;
    unsigned char pixels[16];
    Uint32 before;
    SDL_SetMainReady();
    CHECK(SDL_setenv("SDL_VIDEODRIVER", "flashvm-driver-that-does-not-exist", 1) == 0);
    CHECK(SDL_InitSubSystem(SDL_INIT_VIDEO) != 0);
    SDL_Quit(); /* Discard failed-video SDK bookkeeping before establishing host ownership. */
    CHECK(SDL_InitSubSystem(SDL_INIT_TIMER | SDL_INIT_EVENTS) == 0);
    before = SDL_WasInit(0);
    CHECK(!(before & SDL_INIT_VIDEO));
    CHECK(!fvm_desktop_window_open_offscreen(0, 2));
    CHECK(!fvm_desktop_window_open_offscreen(2, FVM_DESKTOP_MAX_PIXELS + 1));
    CHECK(SDL_WasInit(0) == before);
    memset(&event, 0, sizeof(event));
    event.type = SDL_USEREVENT;
    event.user.code = 719;
    CHECK(SDL_PushEvent(&event) == 1);
    view = fvm_desktop_window_open_offscreen(2, 2);
    CHECK(view);
    CHECK(SDL_WasInit(0) == before);
    memset(&surface, 0, sizeof(surface));
    memset(pixels, 255, sizeof(pixels));
    surface.width = surface.height = 2;
    surface.stride = 8;
    surface.pixels = pixels;
    CHECK(fvm_desktop_window_present(view, &surface, sizeof(pixels)));
    CHECK(!fvm_desktop_window_present(view, &surface, sizeof(pixels) - 1));
    surface.width = 1;
    CHECK(!fvm_desktop_window_present(view, &surface, sizeof(pixels)));
    CHECK(fvm_desktop_window_events(view, 0));
    CHECK(fvm_desktop_window_events(view, 1));
    CHECK(fvm_desktop_window_active(view));
    CHECK(fvm_desktop_window_status(view) == FVM_OK);
    memset(&input, 0xff, sizeof(input));
    fvm_desktop_window_input(view, &input);
    CHECK(!input.buttons && !input.clicks && !input.key_presses);
    CHECK(input.keyboard.struct_size == sizeof(input.keyboard));
    CHECK(fvm_desktop_window_sync_cursor(view, 0) == FVM_INVALID_ARGUMENT);
    CHECK(SDL_PollEvent(&event) == 1 && event.type == SDL_USEREVENT && event.user.code == 719);
    fvm_desktop_window_close(view);
    CHECK(SDL_WasInit(0) == before);
    SDL_QuitSubSystem(SDL_INIT_TIMER | SDL_INIT_EVENTS);
    CHECK(!SDL_WasInit(0));
    puts("PASS timer-only geometry, no video, preserved events and host SDL references");
    return 0;
}
