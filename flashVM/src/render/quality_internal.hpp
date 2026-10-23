/* FlashVM: explicit antialias policy for context-owned MovieClip rendering.
 * Public standalone path APIs retain their existing antialiased behavior. */
#ifndef FVM_RENDER_QUALITY_INTERNAL_HPP
#define FVM_RENDER_QUALITY_INTERNAL_HPP /* Private rendering-quality interfaces. */
#include "flashvm/bitmap_paint.h"
#include "flashvm/gradient.h"
#include "flashvm/path.h"
/* Render vector coverage with per-call AA; no process/global renderer state is changed. */
FVM_Result fvm_path_draw_quality(const FVM_Path *, FVM_RasterSurface *, FVM_RasterColor,
                                 double width, int even_odd, bool antialias);
/* Composite a source image through a path mask with the selected coverage policy. */
FVM_Result fvm_path_mask_quality(const FVM_Path *, FVM_RasterSurface *, const FVM_RasterSurface *,
                                 int even_odd, bool antialias);
/* Sample the existing gradient, then use the selected vector-edge coverage policy. */
FVM_Result fvm_path_draw_gradient_quality(const FVM_Path *, FVM_RasterSurface *,
                                          const FVM_Gradient *, int even_odd, bool antialias);
/* Select the spread mode and per-call edge coverage without changing global raster state. */
FVM_Result fvm_path_draw_gradient_spread_quality(const FVM_Path *, FVM_RasterSurface *,
                                                 const FVM_Gradient *, int even_odd,
                                                 FVM_GradientSpread, bool antialias);
/* Sample existing bitmap paint without changing its smoothing option; select only edge AA. */
FVM_Result fvm_path_draw_bitmap_quality(const FVM_Path *, FVM_RasterSurface *,
                                        const FVM_BitmapPaint *, int even_odd, bool antialias);
#endif
