/* FlashVM: Rendering jobs and drawing integration: raster.
 * File: src/render/raster_c.cpp.
 */

#include "flashvm/raster.h"
#include <climits>
#ifdef FVM_HAS_AGG
#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgba.h"
#include "agg_renderer_base.h"
#include "agg_renderer_scanline.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_scanline_u.h"
#include "agg_path_storage.h"
#endif

#ifdef FVM_HAS_CAIRO
#include "cairo.h"
#include <cstring>
/* Cairo ARGB32 requires a 32-bit unsigned int on the selected ABI. */
typedef char FVM_CairoWordMustBe32Bits[sizeof(unsigned int) == 4 ? 1 : -1];

/* Cairo uses native-endian premultiplied ARGB32. The public FlashVM
 * surface remains straight-alpha RGBA8 on every platform. */
/* Compute and return cairo pixel. */
static unsigned int cairo_pixel(const unsigned char *rgba)
{
    unsigned int a = rgba[3];
    return (a << 24) | (((rgba[0] * a + 127) / 255) << 16) | (((rgba[1] * a + 127) / 255) << 8) |
           ((rgba[2] * a + 127) / 255);
}

/* Fill cairo. */
static FVM_Result cairo_fill(FVM_RasterSurface *surface, const FVM_RasterPoint *points,
                             size_t count, FVM_RasterColor color)
{
    /* An intermediate surface also avoids changing caller bytes on failure. */
    cairo_surface_t *image =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)surface->width, (int)surface->height);
    if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS)
    {
        cairo_surface_destroy(image);
        return FVM_OUT_OF_MEMORY;
    }

    /* Initialization / Инициализация локального состояния. */
    unsigned char *data = cairo_image_surface_get_data(image);
    int stride = cairo_image_surface_get_stride(image);
    for (unsigned int y = 0; y < surface->height; ++y)
        for (unsigned int x = 0; x < surface->width; ++x)
        {
            unsigned int pixel = cairo_pixel(surface->pixels + y * surface->stride + x * 4);
            std::memcpy(data + y * stride + x * 4, &pixel, 4);
        }
    cairo_surface_mark_dirty(image);
    cairo_t *context = cairo_create(image);

    /* Computation / Вычисление и обновление состояния. */
    cairo_set_source_rgba(context, color.r / 255.0, color.g / 255.0, color.b / 255.0,
                          color.a / 255.0);
    cairo_move_to(context, points[0].x, points[0].y);
    for (size_t i = 1; i < count; ++i)
        cairo_line_to(context, points[i].x, points[i].y);

    /* Computation / Вычисление и обновление состояния. */
    cairo_close_path(context);
    cairo_fill(context);
    cairo_status_t status = cairo_status(context);
    cairo_destroy(context);
    if (status != CAIRO_STATUS_SUCCESS)
    {
        cairo_surface_destroy(image);
        return status == CAIRO_STATUS_NO_MEMORY ? FVM_OUT_OF_MEMORY : FVM_ERROR;
    }
    cairo_surface_flush(image);
    for (unsigned int y = 0; y < surface->height; ++y)
        for (unsigned int x = 0; x < surface->width; ++x)
        {
            unsigned char *rgba = surface->pixels + y * surface->stride + x * 4;
            unsigned int pixel;
            std::memcpy(&pixel, data + y * stride + x * 4, 4);
            /* Preserve untouched pixels exactly, including RGB hidden at alpha 0. */
            if (pixel == cairo_pixel(rgba))
                continue;
            unsigned int a = pixel >> 24;
            rgba[3] = (unsigned char)a;
            for (unsigned int c = 0; c < 3; ++c)
            {
                unsigned int component = (pixel >> (16 - c * 8)) & 255;
                unsigned int straight = a ? (component * 255 + a / 2) / a : 0;
                rgba[c] = (unsigned char)(straight > 255 ? 255 : straight);
            }
        }
    cairo_surface_destroy(image);
    return FVM_OK;
}
#endif

/* Fill raster polygon. */
extern "C" FVM_Result fvm_raster_fill_polygon(FVM_RasterSurface *surface,
                                              const FVM_RasterPoint *points, size_t count,
                                              FVM_RasterColor color)
{
    if (!surface || !surface->pixels || !points || count < 3 || surface->width == 0 ||
        surface->height == 0 || surface->width > 4096 || surface->height > 4096 ||
        surface->stride < (size_t)surface->width * 4 ||
        surface->stride > (size_t)INT_MAX / surface->height ||
        surface->stride > (size_t)-1 / surface->height)
        return FVM_INVALID_ARGUMENT;
    for (size_t i = 0; i < count; ++i)
    {
        /* Comparisons reject NaN as well as infinity without C99 isfinite. */
        if (!(points[i].x >= -4096.0 && points[i].x <= 4096.0) ||
            !(points[i].y >= -4096.0 && points[i].y <= 4096.0))
            return FVM_INVALID_ARGUMENT;
    }
#ifdef FVM_HAS_AGG
    try
    {
        agg::rendering_buffer buffer(surface->pixels, surface->width, surface->height,
                                     (int)surface->stride);
        agg::pixfmt_rgba32_plain pixels(buffer);
        agg::renderer_base<agg::pixfmt_rgba32_plain> renderer(pixels);
        agg::rasterizer_scanline_aa<> rasterizer;
        agg::scanline_u8 scanline;
        agg::path_storage path;
        path.move_to(points[0].x, points[0].y);
        for (size_t i = 1; i < count; ++i)
            path.line_to(points[i].x, points[i].y);
        path.close_polygon();
        rasterizer.clip_box(0, 0, surface->width, surface->height);
        rasterizer.add_path(path);
        agg::render_scanlines_aa_solid(rasterizer, scanline, renderer,
                                       agg::rgba8(color.r, color.g, color.b, color.a));
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
#elif defined(FVM_HAS_CAIRO)
    return cairo_fill(surface, points, count, color);
#else
    (void)color;
    return FVM_UNSUPPORTED;
#endif
}
