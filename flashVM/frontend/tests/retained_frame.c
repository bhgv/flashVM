/* Real SDL dummy renderer: copied frame lifetime, pitch and rejected input. */
#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>

static int fail_create;
static unsigned creates;
static int fail_draw;


static int test_copy(SDL_Renderer *r, SDL_Texture *t, const SDL_Rect *a, const SDL_Rect *b)
{
    if (fail_draw)
    {
        SDL_SetError("injected repaint failure");
        return -1;
    }
    return SDL_RenderCopy(r, t, a, b);
}


static SDL_Texture *test_create(SDL_Renderer *r, Uint32 f, int a, int w, int h)
{
    ++creates;
    if (fail_create)
    {
        SDL_SetError("injected texture allocation failure");
        return NULL;
    }
    return SDL_CreateTexture(r, f, a, w, h);
}

#define SDL_RenderCopy test_copy
#define SDL_CreateTexture test_create
#include "../common/sdl_window.c"

#undef SDL_CreateTexture
#undef SDL_RenderCopy


int main(void)
{
    FVM_DesktopWindow *view;
    FVM_RasterSurface surface;
    unsigned char source[24] = {1, 2, 3, 255, 4,  5,  6,  255, 99, 99, 99, 99,
                                7, 8, 9, 255, 10, 11, 12, 255, 99, 99, 99, 99};
    const unsigned char expected[16] = {1, 2, 3, 255, 4, 5, 6, 255, 7, 8, 9, 255, 10, 11, 12, 255};
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
    view = fvm_desktop_window_open("retained-frame", 2, 2);
    if (!(view))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        SDL_Event event;
        memset(&event, 0, sizeof(event));
        event.type = SDL_RENDER_DEVICE_RESET;
        if (!(handle_event(view, &event)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!view->ready && !view->last_frame && !view->texture_invalid))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_window_status(view) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    memset(&surface, 0, sizeof(surface));
    surface.pixels = source;
    surface.width = 2;
    surface.height = 2;
    surface.stride = 12;
    if (!(fvm_desktop_window_present(view, &surface, sizeof(source))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(view->last_frame_size == sizeof(expected)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(view->last_frame, expected, sizeof(expected))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        SDL_Event event;
        unsigned char pixels[16];
        memset(&event, 0, sizeof(event));
        event.type = SDL_RENDER_DEVICE_RESET;
        if (!(handle_event(view, &event)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_window_status(view) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(SDL_RenderClear(view->renderer) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(SDL_RenderCopy(view->renderer, view->texture, NULL, NULL) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(SDL_RenderReadPixels(view->renderer, NULL, SDL_PIXELFORMAT_RGBA32, pixels, 8) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!memcmp(pixels, expected, sizeof(expected))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        event.type = SDL_RENDER_TARGETS_RESET;
        if (!(handle_event(view, &event)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_window_status(view) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_window_status(NULL) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    {
        SDL_Event event;
        SDL_Texture *old = view->texture;
        unsigned before = creates;
        memset(&event, 0, sizeof(event));
        event.type = SDL_APP_WILLENTERBACKGROUND;
        if (!(handle_event(view, &event)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        event.type = SDL_RENDER_DEVICE_RESET;
        if (!(handle_event(view, &event)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(handle_event(view, &event)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(view->texture_invalid && view->texture == old && creates == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        event.type = SDL_APP_WILLENTERFOREGROUND;
        if (!(handle_event(view, &event)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(view->texture_invalid && creates == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        event.type = SDL_APP_DIDENTERFOREGROUND;
        if (!(handle_event(view, &event)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!view->texture_invalid && creates == before + 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_window_status(view) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!memcmp(view->last_frame, expected, sizeof(expected))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    memset(source, 0, sizeof(source));
    if (!(!memcmp(view->last_frame, expected, sizeof(expected))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    surface.width = 1;
    if (!(!fvm_desktop_window_present(view, &surface, sizeof(source))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(view->last_frame, expected, sizeof(expected))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    surface.width = 2;
    if (!(!fvm_desktop_window_present(view, &surface, 1)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(view->last_frame, expected, sizeof(expected))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_present(view, &surface, sizeof(source))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(view->last_frame[0] == 0 && view->last_frame[15] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        SDL_Event event;
        SDL_Texture *old = view->texture;
        memset(&event, 0, sizeof(event));
        event.type = SDL_RENDER_DEVICE_RESET;
        fail_create = 1;
        if (!(handle_event(view, &event)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(view->texture == old))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_window_status(view) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_create = 0;
        if (!(handle_event(view, &event)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_window_status(view) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_desktop_window_close(view);
    view = fvm_desktop_window_open("repaint-error", 2, 2);
    if (!(view))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_present(view, &surface, sizeof(source))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        SDL_Event event;
        memset(&event, 0, sizeof(event));
        event.type = SDL_WINDOWEVENT;
        event.window.event = SDL_WINDOWEVENT_EXPOSED;
        fail_draw = 1;
        if (!(handle_event(view, &event)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_desktop_window_status(view) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_draw = 0;
    }
    fvm_desktop_window_close(view);
    return 0;
}
