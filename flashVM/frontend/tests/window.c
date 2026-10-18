/* Exercise the actual SDL adapter on a native display, including byte-order-correct colors and
 * close. */
#include "../common/sdl_window.c"
#include <assert.h>
#include <string.h>
int main(void)
{
    FVM_DesktopWindow *view;
    unsigned char source[16] = {255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
    unsigned char result[16];
    FVM_RasterSurface surface;
    SDL_Event event;
    surface.pixels = source;
    surface.width = 2;
    surface.height = 2;
    surface.stride = 8;
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
    view = fvm_desktop_window_open("FlashVM presentation test", 2, 2);
    assert(view);
    assert(fvm_desktop_window_present(view, &surface));
    /* Redraw/read before present: SDL does not guarantee backbuffer contents after present. */
    assert(SDL_RenderCopy(view->renderer, view->texture, 0, 0) == 0);
    assert(SDL_RenderReadPixels(view->renderer, 0, SDL_PIXELFORMAT_RGBA32, result, 8) == 0);
    assert(!memcmp(source, result, sizeof(source)));
    assert(fvm_desktop_window_events(view, 2));
    memset(&event, 0, sizeof(event));
    event.type = SDL_WINDOWEVENT;
    event.window.event = SDL_WINDOWEVENT_CLOSE;
    assert(SDL_PushEvent(&event) == 1);
    assert(!fvm_desktop_window_events(view, 0));
    fvm_desktop_window_close(view);
    fvm_desktop_window_close(0);
    return 0;
}
