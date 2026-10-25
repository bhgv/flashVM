/* FlashVM: C embedding API: svg.
 * File: src/capi/svg_c.cpp.
 */

#include "flashvm/svg.h"

#ifdef FVM_HAS_IMAGE_CODECS
#include "nanosvg.h"
#include "nanosvgrast.h"
#include <climits>
#include <cstring>
#include <new>


/* Define FVM_SVG struct and its representation. */
struct FVM_SVG
{
    NSVGimage *image;
    NSVGrasterizer *rasterizer;
};

/* Reject non-finite/extreme transformed geometry before the rasterizer converts
 * coordinates to integer scanline positions. This is an embedding bound, not
 * a general SVG validator. */


/* Compute and return valid geometry. */
static bool valid_geometry(NSVGimage *image)
{
    size_t total_points = 0;
    for (NSVGshape *shape = image->shapes; shape; shape = shape->next)
    {
        for (NSVGpath *path = shape->paths; path; path = path->next)
        {
            if ((path->npts && !path->pts) || path->npts < 0 || (size_t)path->npts > 1000000 - total_points)
            {
                return false;
            }
            total_points += (size_t)path->npts;
            for (int i = 0; i < path->npts * 2; ++i)
            {
                if (!(path->pts[i] >= -1000000.0f && path->pts[i] <= 1000000.0f))
                {
                    return false;
                }
            }
        }
    }
    return true;
}


/* Read svg. */
extern "C" FVM_Result fvm_svg_read(const char *data, size_t size, FVM_SVG **out)
{
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = 0;
    if (!data || !size || size > 4 * 1024 * 1024 || std::memchr(data, 0, size))
    {
        return FVM_INVALID_ARGUMENT;
    }
    char *copy = new (std::nothrow) char[size + 1];
    if (!copy)
    {
        return FVM_OUT_OF_MEMORY;
    }


    /* Computation / Вычисление и обновление состояния. */
    std::memcpy(copy, data, size);
    copy[size] = 0;
    NSVGimage *image = nsvgParse(copy, "px", 96.0f);
    delete[] copy;
    if (!image)
    {
        return FVM_ERROR;
    }
    if (!(image->width > 0 && image->width <= 4096) || !(image->height > 0 && image->height <= 4096) ||
        !valid_geometry(image))
    {
        nsvgDelete(image);
        return FVM_INVALID_ARGUMENT;
    }
    FVM_SVG *svg = new (std::nothrow) FVM_SVG;
    if (!svg)
    {
        nsvgDelete(image);
        return FVM_OUT_OF_MEMORY;
    }

    /* Computation / Вычисление и обновление состояния. */
    svg->image = image;
    svg->rasterizer = nsvgCreateRasterizer();
    if (!svg->rasterizer)
    {
        nsvgDelete(image);
        delete svg;
        return FVM_OUT_OF_MEMORY;
    }
    *out = svg;
    return FVM_OK;
}


/* Compute and return svg size. */
extern "C" FVM_Result fvm_svg_size(const FVM_SVG *svg, double *width, double *height)
{
    if (!svg || !width || !height)
    {
        return FVM_INVALID_ARGUMENT;
    }

    /* Computation / Вычисление и обновление состояния. */
    *width = svg->image->width;
    *height = svg->image->height;
    return FVM_OK;
}


/* Compute and return svg rasterize. */
extern "C" FVM_Result fvm_svg_rasterize(FVM_SVG *svg, FVM_RasterSurface *surface)
{
    if (!svg || !surface || !surface->pixels || !surface->width || !surface->height || surface->width > 4096 ||
        surface->height > 4096 || surface->stride < (size_t)surface->width * 4 ||
        surface->stride > (size_t)INT_MAX / surface->height)
    {
        return FVM_INVALID_ARGUMENT;
    }
    nsvgRasterize(svg->rasterizer,
                  svg->image,
                  0,
                  0,
                  1,
                  surface->pixels,
                  (int)surface->width,
                  (int)surface->height,
                  (int)surface->stride);
    return FVM_OK;
}


/* Destroy svg. */
extern "C" void fvm_svg_destroy(FVM_SVG *svg)
{
    if (svg)
    {
        nsvgDeleteRasterizer(svg->rasterizer);
        nsvgDelete(svg->image);
        delete svg;
    }
}
#else
/* Keep the facade linkable when NO_IMAGE_CODECS is selected. */
/* Read svg. */
extern "C" FVM_Result fvm_svg_read(const char *, size_t, FVM_SVG **out)
{
    if (!out)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *out = 0;
    return FVM_UNSUPPORTED;
}


/* Compute and return svg size. */
extern "C" FVM_Result fvm_svg_size(const FVM_SVG *, double *, double *)
{
    return FVM_UNSUPPORTED;
}


/* Compute and return svg rasterize. */
extern "C" FVM_Result fvm_svg_rasterize(FVM_SVG *, FVM_RasterSurface *)
{
    return FVM_UNSUPPORTED;
}


/* Destroy svg. */
extern "C" void fvm_svg_destroy(FVM_SVG *)
{
}
#endif
