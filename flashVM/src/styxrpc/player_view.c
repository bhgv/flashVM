/* Copyright (c) 2026 bhgv
 * EN: Present paired SWFs with the stock desktop input and raster paths.
 * RU: Отрисовка парного SWF штатным растером и обработкой событий рабочего стола.
 */
#define _POSIX_C_SOURCE 200809L /* mkstemp, fdopen and reliable output path replacement. */
#include "player_view.h"
#include "playback_clock.h"
#include "flashvm/movieclip.h"
#include "flashvm/stage.h"
#include "flashvm/swf.h"
#include "../../ports/monotonic_port.h"
#ifdef FVM_STYXRPC_WINDOW
#include "window.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PAIRED_RGBA_BYTES 4U         /* Pixel width of FlashVM raster buffers. */
#define PAIRED_RGB_BYTES 3U          /* Pixel width of binary PPM output. */
#define PAIRED_MAX_PIXELS 4096U      /* Limit per dimension of the existing raster backend. */
#define PAIRED_WINDOW_PERIOD_MS 16  /* Upper bound for idle native window event polling. */

struct FVM_PairedView
{
    FVM_RasterSurface surface; /* Owned reusable raster storage. */
    const char *capture;       /* Borrowed PPM path, NULL without capture. */
#ifdef FVM_STYXRPC_WINDOW
    FVM_DesktopWindow *window; /* Owned on the same thread as the VM. */
    double next;              /* Next monotonic presentation deadline in milliseconds. */
    int windowed;             /* Open a window after the first movie arrives. */
#endif
};


/* EN/RU: See player_view.h. / См. player_view.h. */
FVM_PairedView *fvm_paired_view_create(int windowed, const char *capture)
{
    FVM_PairedView *view;
#ifndef FVM_STYXRPC_RASTER
    if (windowed || capture)
    {
        fputs("flashvm_styx_player: presentation requires GRAPHICS_AGG or GRAPHICS_CAIRO\n", stderr);
        return NULL;
    }
#endif
#ifndef FVM_STYXRPC_WINDOW
    if (windowed)
    {
        fputs("flashvm_styx_player: --window requires the SDL2 desktop frontend\n", stderr);
        return NULL;
    }
#endif
    view = calloc(1, sizeof(*view));
    if (view)
    {
        view->capture = capture;
#ifdef FVM_STYXRPC_WINDOW
        view->windowed = windowed;
#endif
    }
    return view;
}


/* EN: Allocate the native-size surface and copy the translation from authored twips.
 * RU: Выделить буфер размера сцены и скопировать смещение из авторских твипов. */
static int stage_surface(FVM_PairedView *view, FVM_Context *context, FVM_Affine *viewport)
{
    FVM_SWFStageBounds bounds;
    double width, height;
    unsigned int w, h;
    unsigned char *pixels;
    bounds.struct_size = sizeof(bounds);
    if (fvm_swf_stage_bounds(context, &bounds) != FVM_OK)
    {
        return 0;
    }
    width = ((double)bounds.x_max - bounds.x_min) / FVM_SWF_TWIPS_PER_PIXEL;
    height = ((double)bounds.y_max - bounds.y_min) / FVM_SWF_TWIPS_PER_PIXEL;
    if (!(width > 0 && height > 0 && width <= PAIRED_MAX_PIXELS && height <= PAIRED_MAX_PIXELS))
    {
        return 0;
    }
    w = (unsigned int)width;
    h = (unsigned int)height;
    w += w < width;
    h += h < height;
    if (w != view->surface.width || h != view->surface.height)
    {
        pixels = realloc(view->surface.pixels, (size_t)w * h * PAIRED_RGBA_BYTES);
        if (!pixels)
        {
            return 0;
        }
        view->surface.pixels = pixels;
        view->surface.width = w;
        view->surface.height = h;
        view->surface.stride = (size_t)w * PAIRED_RGBA_BYTES;
    }
    viewport->xx = viewport->yy = 1;
    viewport->xy = viewport->yx = 0;
    viewport->x0 = -(double)bounds.x_min / FVM_SWF_TWIPS_PER_PIXEL;
    viewport->y0 = -(double)bounds.y_min / FVM_SWF_TWIPS_PER_PIXEL;
    return 1;
}


/* EN: Clear the surface to the current stage color, then render the live scene.
 * RU: Очистить буфер цветом сцены, затем отрисовать живое представление. */
static int render(FVM_RasterSurface *surface, FVM_Context *context, FVM_Object *root, const FVM_Affine *viewport)
{
    FVM_RasterColor background = {255, 255, 255, 255};
    FVM_Result status = fvm_stage_get_background(context, &background);
    unsigned int x, y;
    if (status != FVM_OK && status != FVM_NOT_FOUND)
    {
        return 0;
    }
    for (y = 0; y < surface->height; ++y)
    {
        unsigned char *pixel = surface->pixels + y * surface->stride;
        for (x = 0; x < surface->width; ++x, pixel += PAIRED_RGBA_BYTES)
        {
            pixel[0] = background.r;
            pixel[1] = background.g;
            pixel[2] = background.b;
            pixel[3] = background.a;
        }
    }
    return fvm_movieclip_render_view(context, root, surface, viewport) == FVM_OK;
}


/* EN/RU: See player_view.h. / См. player_view.h. */
int fvm_paired_view_step(FVM_PairedView *view, FVM_StyxRpcPlayback *playback, int *wait_ms)
{
#ifdef FVM_STYXRPC_WINDOW
    FVM_Context *context;
    FVM_Object *root;
    FVM_Affine viewport;
    double now;
    unsigned int previous_width, previous_height;
    if (!view || !view->windowed || !fvm_styxrpc_playback_scene(playback, &context, &root))
    {
        return 1;
    }
    if (!fvm_port_monotonic_ms(&now))
    {
        return -1;
    }
    if (*wait_ms < 0 || *wait_ms > PAIRED_WINDOW_PERIOD_MS)
    {
        *wait_ms = PAIRED_WINDOW_PERIOD_MS;
    }
    if (now < view->next)
    {
        return 1;
    }
    view->next = now + PAIRED_WINDOW_PERIOD_MS;
    previous_width = view->surface.width;
    previous_height = view->surface.height;
    if (!stage_surface(view, context, &viewport))
    {
        return -1;
    }
    if (view->window && (previous_width != view->surface.width || previous_height != view->surface.height))
    {
        fvm_desktop_window_close(view->window);
        view->window = NULL;
    }
    if (!view->window)
    {
        view->window = fvm_desktop_window_open("FlashVM Styx", view->surface.width, view->surface.height);
        if (!view->window)
        {
            return -1;
        }
    }
    if (!fvm_desktop_window_events(view->window, 0))
    {
        return fvm_desktop_window_status(view->window) == FVM_OK ? 0 : -1;
    }
    if (fvm_desktop_window_pump_clip_input(view->window, context, root, -viewport.x0, -viewport.y0) != FVM_OK ||
        !render(&view->surface, context, root, &viewport) ||
        !fvm_desktop_window_present(view->window, &view->surface, view->surface.stride * view->surface.height) ||
        fvm_desktop_window_sync_cursor(view->window, context) != FVM_OK)
    {
        return -1;
    }
#else
    (void)view;
    (void)playback;
    (void)wait_ms;
#endif
    return 1;
}


/* EN: Write exactly one raster to the borrowed PPM stream. Return one on success.
 * RU: Записать один растр в заимствованный поток PPM. Вернуть один при успехе. */
static int write_ppm(FILE *file, const FVM_RasterSurface *surface)
{
    unsigned char row[PAIRED_MAX_PIXELS * PAIRED_RGB_BYTES];
    unsigned int x, y;
    if (fprintf(file, "P6\n%u %u\n255\n", surface->width, surface->height) < 0)
    {
        return 0;
    }
    for (y = 0; y < surface->height; ++y)
    {
        const unsigned char *source = surface->pixels + y * surface->stride;
        for (x = 0; x < surface->width; ++x)
        {
            memcpy(row + x * PAIRED_RGB_BYTES, source + x * PAIRED_RGBA_BYTES, PAIRED_RGB_BYTES);
        }
        if (fwrite(row, PAIRED_RGB_BYTES, surface->width, file) != surface->width)
        {
            return 0;
        }
    }
    return 1;
}


/* EN/RU: See player_view.h. / См. player_view.h. */
int fvm_paired_view_capture(FVM_PairedView *view, FVM_StyxRpcPlayback *playback)
{
    static const char suffix[] = ".tmp.XXXXXX";
    FVM_Context *context;
    FVM_Object *root;
    FVM_Affine viewport;
    size_t length;
    char *temporary;
    FILE *file;
    int fd, good = 0;
    if (!view || !view->capture)
    {
        return 1;
    }
    if (!fvm_styxrpc_playback_scene(playback, &context, &root) ||
        !stage_surface(view, context, &viewport) || !render(&view->surface, context, root, &viewport))
    {
        return 0;
    }
    length = strlen(view->capture);
    temporary = malloc(length + sizeof(suffix));
    if (!temporary)
    {
        return 0;
    }
    memcpy(temporary, view->capture, length);
    memcpy(temporary + length, suffix, sizeof(suffix));
    fd = mkstemp(temporary);
    if (fd >= 0)
    {
        file = fdopen(fd, "wb");
        if (file)
        {
            good = write_ppm(file, &view->surface);
            if (fclose(file))
            {
                good = 0;
            }
        }
        else
        {
            close(fd);
        }
        good = good && rename(temporary, view->capture) == 0;
        if (!good)
        {
            unlink(temporary);
        }
    }
    free(temporary);
    return good;
}


/* EN/RU: See player_view.h. / См. player_view.h. */
void fvm_paired_view_destroy(FVM_PairedView *view)
{
    if (view)
    {
#ifdef FVM_STYXRPC_WINDOW
        fvm_desktop_window_close(view->window);
#endif
        free(view->surface.pixels);
        free(view);
    }
}
