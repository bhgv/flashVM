/* SDL2 window adapter for desktop targets. All event/render calls remain on main thread. */
#define SDL_MAIN_HANDLED /* OS entrypoints are supplied in frontend/{linux,windows,macos}. */
#include <SDL.h>
#include <stdlib.h>
#include "window.h"
/* Own presentation objects in reverse-destruction order; retain no VM pointers. */
struct FVM_DesktopWindow
{
    SDL_Window *window;     /* Native OS window, owned until close. */
    SDL_Renderer *renderer; /* SDL software/accelerated presenter selected by SDL. */
    SDL_Texture *texture;   /* Streaming byte-order-independent RGBA texture. */
    int ready; /* Nonzero after a complete upload; permits repaint without another VM tick. */
};

/* Create a fixed logical stage and an independently resizable OS window. */
FVM_DesktopWindow *fvm_desktop_window_open(const char *title, unsigned int width,
                                           unsigned int height)
{
    FVM_DesktopWindow *view;
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
        return 0;
    view = (FVM_DesktopWindow *)calloc(1, sizeof(*view));
    if (!view)
    {
        SDL_Quit();
        return 0;
    }
    view->window =
        SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int)width,
                         (int)height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (view->window)
        view->renderer = SDL_CreateRenderer(view->window, -1, 0);
    if (view->renderer)
    {
        if (SDL_RenderSetLogicalSize(view->renderer, (int)width, (int)height) == 0)
            view->texture = SDL_CreateTexture(view->renderer, SDL_PIXELFORMAT_RGBA32,
                                              SDL_TEXTUREACCESS_STREAMING, (int)width, (int)height);
    }
    if (!view->window || !view->renderer || !view->texture)
    {
        /* Keep SDL's diagnostic visible before destroying objects or quitting the subsystem. */
        SDL_Log("FlashVM window initialization: %s", SDL_GetError());
        fvm_desktop_window_close(view);
        return 0;
    }
    if (SDL_SetTextureBlendMode(view->texture, SDL_BLENDMODE_NONE) != 0)
    {
        SDL_Log("FlashVM texture blend mode: %s", SDL_GetError());
        fvm_desktop_window_close(view);
        return 0;
    }
    return view;
}

/* Present the already uploaded image, preserving aspect and clearing letterbox margins. */
static int draw_texture(FVM_DesktopWindow *view)
{
    if (SDL_SetRenderDrawColor(view->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE) != 0)
        return 0;
    if (SDL_RenderClear(view->renderer) != 0)
        return 0;
    if (SDL_RenderCopy(view->renderer, view->texture, 0, 0) != 0)
        return 0;
    SDL_RenderPresent(view->renderer);
    return 1;
}

/* Render the complete logical stage; caller storage is never retained by this adapter. */
int fvm_desktop_window_present(FVM_DesktopWindow *view, const FVM_RasterSurface *surface)
{
    if (SDL_UpdateTexture(view->texture, 0, surface->pixels, (int)surface->stride) != 0)
        return 0;
    view->ready = 1;
    return draw_texture(view);
}

/* Handle OS events without advancing the VM. Expose/resize can repaint during a slow frame. */
static int handle_event(FVM_DesktopWindow *view, const SDL_Event *event)
{
    if (event->type == SDL_QUIT ||
        (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_CLOSE) ||
        (event->type == SDL_KEYDOWN && event->key.keysym.sym == SDLK_ESCAPE))
        return 0;
    if (view->ready && event->type == SDL_WINDOWEVENT &&
        (event->window.event == SDL_WINDOWEVENT_EXPOSED ||
         event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED))
        draw_texture(view);
    return 1;
}

/* SDL_WaitEventTimeout removes an event: inspect it instead of accidentally losing close. */
int fvm_desktop_window_events(FVM_DesktopWindow *view, int timeout_ms)
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
        if (!handle_event(view, &event))
            return 0;
    if (timeout_ms > 0 && SDL_WaitEventTimeout(&event, timeout_ms))
        return handle_event(view, &event);
    return 1;
}

/* SDL counters are monotonic and keep scheduling independent of adjustable wall-clock dates. */
double fvm_desktop_now(void)
{
    return (double)SDL_GetPerformanceCounter() * FVM_DESKTOP_MILLISECONDS /
           (double)SDL_GetPerformanceFrequency();
}

/* Borrow the current SDL error string for immediate reporting only. */
const char *fvm_desktop_window_error(void)
{
    return SDL_GetError();
}

/* Safe reverse-order cleanup also handles partially initialized windows. */
void fvm_desktop_window_close(FVM_DesktopWindow *view)
{
    if (!view)
        return;
    if (view->texture)
        SDL_DestroyTexture(view->texture);
    if (view->renderer)
        SDL_DestroyRenderer(view->renderer);
    if (view->window)
        SDL_DestroyWindow(view->window);
    free(view);
    SDL_Quit();
}
