/* Real SDL dummy renderer: copied frame lifetime, pitch and rejected input. */
#include "../common/sdl_window.c"
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
    fvm_desktop_window_close(view);
    return 0;
}
