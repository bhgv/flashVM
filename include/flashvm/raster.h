#ifndef FLASHVM_RASTER_H
#define FLASHVM_RASTER_H
#include "flashvm.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Host-owned RGBA8 straight-alpha image. The buffer has at least
     * stride * height bytes. Coordinates are pixels, not SWF twips. */
    typedef struct FVM_RasterSurface
    {
        unsigned char *pixels;
        unsigned int width;
        unsigned int height;
        size_t stride;
    } FVM_RasterSurface;
    typedef struct FVM_RasterPoint
    {
        double x, y;
    } FVM_RasterPoint;
    typedef struct FVM_RasterColor
    {
        unsigned char r, g, b, a;
    } FVM_RasterColor;
    /* Antialiased, nonzero winding fill; clipped to the image. No OS calls.
     * Coordinates must be finite and within +/- 4096 pixels.
     * GRAPHICS_AGG / GRAPHICS_CAIRO select the bundled backend in DEFS.txt.
     * Returns UNSUPPORTED with NO_GRAPHICS. */
    FVM_API FVM_Result fvm_raster_fill_polygon(FVM_RasterSurface *, const FVM_RasterPoint *, size_t,
                                               FVM_RasterColor);
#ifdef __cplusplus
}
#endif
#endif
