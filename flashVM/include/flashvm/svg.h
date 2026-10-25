/* FlashVM: Public embedding API declarations: svg.
 * File: include/flashvm/svg.h.
 */

#ifndef FLASHVM_SVG_H
/* Define FLASHVM SVG H constant or compilation switch. */
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


/* Read svg. */
FVM_API FVM_Result fvm_svg_read(const char *data, size_t size, FVM_SVG **out);


/* Implement svg size for this subsystem. */
FVM_API FVM_Result fvm_svg_size(const FVM_SVG *svg, double *width, double *height);
/* Render at scale 1 in host-owned straight RGBA8. Only pixel bytes are cleared;
     * row padding is preserved. The surface may crop the SVG viewport. */


/* Implement svg rasterize for this subsystem. */
FVM_API FVM_Result fvm_svg_rasterize(FVM_SVG *svg, FVM_RasterSurface *surface);


/* Destroy svg. */
FVM_API void fvm_svg_destroy(FVM_SVG *svg);
#ifdef __cplusplus
}
#endif
#endif
