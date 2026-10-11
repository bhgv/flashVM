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
#else
    (void)color;
    return FVM_UNSUPPORTED;
#endif
}
