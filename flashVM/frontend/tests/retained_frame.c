/* Real SDL dummy renderer: copied frame lifetime, pitch and rejected input. */
#include <SDL.h>
static int fail_create;
static unsigned creates;
static SDL_Texture *test_create(SDL_Renderer *r, Uint32 f, int a, int w, int h)
{
    ++creates;
    if (fail_create) { SDL_SetError("injected texture allocation failure"); return NULL; }
    return SDL_CreateTexture(r, f, a, w, h);
}
#define SDL_CreateTexture test_create
#include "../common/sdl_window.c"
#undef SDL_CreateTexture
#include <assert.h>
int main(void)
{
    FVM_DesktopWindow *view;
    FVM_RasterSurface surface;
    unsigned char source[24] = {1,2,3,255,4,5,6,255,99,99,99,99,
                               7,8,9,255,10,11,12,255,99,99,99,99};
    const unsigned char expected[16] = {1,2,3,255,4,5,6,255,7,8,9,255,10,11,12,255};
    assert(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0);
    assert(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software"));
    view = fvm_desktop_window_open("retained-frame", 2, 2);
    assert(view);
    memset(&surface, 0, sizeof(surface));
    surface.pixels = source; surface.width = 2; surface.height = 2; surface.stride = 12;
    assert(fvm_desktop_window_present(view, &surface, sizeof(source)));
    assert(view->last_frame_size == sizeof(expected));
    assert(!memcmp(view->last_frame, expected, sizeof(expected)));
    {
        SDL_Event event;
        unsigned char pixels[16];
        memset(&event, 0, sizeof(event));
        event.type = SDL_RENDER_DEVICE_RESET;
        assert(handle_event(view, &event));
        assert(fvm_desktop_window_status(view) == FVM_OK);
        assert(SDL_RenderClear(view->renderer) == 0);
        assert(SDL_RenderCopy(view->renderer, view->texture, NULL, NULL) == 0);
        assert(SDL_RenderReadPixels(view->renderer, NULL, SDL_PIXELFORMAT_RGBA32, pixels, 8) == 0);
        assert(!memcmp(pixels, expected, sizeof(expected)));
        event.type = SDL_RENDER_TARGETS_RESET;
        assert(handle_event(view, &event));
        assert(fvm_desktop_window_status(view) == FVM_OK);
        assert(fvm_desktop_window_status(NULL) == FVM_INVALID_ARGUMENT);
    }
    {
        SDL_Event event;
        SDL_Texture *old = view->texture;
        unsigned before = creates;
        memset(&event, 0, sizeof(event)); event.type = SDL_APP_WILLENTERBACKGROUND;
        assert(handle_event(view, &event));
        event.type = SDL_RENDER_DEVICE_RESET;
        assert(handle_event(view, &event)); assert(handle_event(view, &event));
        assert(view->texture_invalid && view->texture == old && creates == before);
        event.type = SDL_APP_WILLENTERFOREGROUND;
        assert(handle_event(view, &event));
        assert(view->texture_invalid && creates == before);
        event.type = SDL_APP_DIDENTERFOREGROUND;
        assert(handle_event(view, &event));
        assert(!view->texture_invalid && creates == before + 1);
        assert(fvm_desktop_window_status(view) == FVM_OK);
        assert(!memcmp(view->last_frame, expected, sizeof(expected)));
    }
    memset(source, 0, sizeof(source));
    assert(!memcmp(view->last_frame, expected, sizeof(expected)));
    surface.width = 1;
    assert(!fvm_desktop_window_present(view, &surface, sizeof(source)));
    assert(!memcmp(view->last_frame, expected, sizeof(expected)));
    surface.width = 2;
    assert(!fvm_desktop_window_present(view, &surface, 1));
    assert(!memcmp(view->last_frame, expected, sizeof(expected)));
    assert(fvm_desktop_window_present(view, &surface, sizeof(source)));
    assert(view->last_frame[0] == 0 && view->last_frame[15] == 0);
    {
        SDL_Event event;
        SDL_Texture *old = view->texture;
        memset(&event, 0, sizeof(event)); event.type = SDL_RENDER_DEVICE_RESET;
        fail_create = 1;
        assert(handle_event(view, &event));
        assert(view->texture == old);
        assert(fvm_desktop_window_status(view) == FVM_ERROR);
        fail_create = 0;
        assert(handle_event(view, &event));
        assert(fvm_desktop_window_status(view) == FVM_ERROR);
    }
    fvm_desktop_window_close(view);
    return 0;
}
