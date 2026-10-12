/* FlashVM: Public embedding API declarations: raster.
 * File: include/flashvm/raster.h.
 */

#ifndef FLASHVM_RASTER_H
/* Define FLASHVM RASTER H constant or compilation switch. */
#define FLASHVM_RASTER_H
#include "flashvm.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Host-owned RGBA8 straight-alpha image. The buffer has at least
     * stride * height bytes. Coordinates are pixels, not SWF twips. */
    /* Define FVM_RasterSurface struct and its representation. */
    typedef struct FVM_RasterSurface
    {
        unsigned char *pixels; /* Pixel buffer for the raster surface. */
        unsigned int width;    /* Horizontal extent in the API coordinate system. */
        unsigned int height;   /* Vertical extent in the API coordinate system. */
        size_t stride;         /* Byte distance between adjacent pixel rows. */
    } FVM_RasterSurface;
    /* Define FVM_RasterPoint struct and its representation. */
    typedef struct FVM_RasterPoint
    {
        double x /* Horizontal coordinate. */, y; /* Vertical coordinate. */
    } FVM_RasterPoint;
    /* Define FVM_RasterColor struct and its representation. */
    typedef struct FVM_RasterColor
    {
        unsigned char r /* Red channel component. */, g /* Green channel component. */,
            b /* Blue channel component. */, a; /* Alpha channel component. */
    } FVM_RasterColor;
    /* Antialiased, nonzero winding fill; clipped to the image. No OS calls.
     * Coordinates must be finite and within +/- 4096 pixels.
     * GRAPHICS_AGG / GRAPHICS_CAIRO select the bundled backend in DEFS.txt.
     * Returns UNSUPPORTED with NO_GRAPHICS. */
    /* Fill raster polygon. */
    FVM_API FVM_Result fvm_raster_fill_polygon(FVM_RasterSurface *, const FVM_RasterPoint *, size_t,
                                               FVM_RasterColor);
#ifdef __cplusplus
}
#endif
#endif
