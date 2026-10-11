#ifndef FLASHVM_SVG_H
#define FLASHVM_SVG_H
#include "flashvm/raster.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* C89 facade for the bundled NanoSVG C++98 reader/rasterizer.
     * It supports a documented static SVG subset, not complete SVG/XML validation.
     * Input is copied. Maximum input: 4 MiB; dimensions: (0, 4096]. */
    typedef struct FVM_SVG FVM_SVG;
    FVM_API FVM_Result fvm_svg_read(const char *data, size_t size, FVM_SVG **out);
    FVM_API FVM_Result fvm_svg_size(const FVM_SVG *svg, double *width, double *height);
    /* Render at scale 1 in host-owned straight RGBA8. Only pixel bytes are cleared;
     * row padding is preserved. The surface may crop the SVG viewport. */
    FVM_API FVM_Result fvm_svg_rasterize(FVM_SVG *svg, FVM_RasterSurface *surface);
    FVM_API void fvm_svg_destroy(FVM_SVG *svg);
#ifdef __cplusplus
}
#endif
#endif
