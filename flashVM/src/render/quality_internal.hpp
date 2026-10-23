/* FlashVM: explicit antialias policy for context-owned MovieClip rendering.
 * Public standalone path APIs retain their existing antialiased behavior. */
#ifndef FVM_RENDER_QUALITY_INTERNAL_HPP
#define FVM_RENDER_QUALITY_INTERNAL_HPP /* Private rendering-quality interfaces. */
#include "flashvm/bitmap_paint.h"
#include "flashvm/gradient.h"
#include "flashvm/path.h"
#include "stroke_constants.h"
/* Test geometry with selected caps/joins, independently of raster availability or quality. */
FVM_Result fvm_path_hit_test_style(const FVM_Path *, double x, double y, double width,
                                   int even_odd, int *out, unsigned cap, unsigned join);
/* Render vector coverage with per-call AA; no process/global renderer state is changed. */
FVM_Result fvm_path_draw_quality(const FVM_Path *, FVM_RasterSurface *, FVM_RasterColor,
                                 double width, int even_odd, bool antialias,
                                 unsigned cap = FVM_STROKE_CAP_ROUND,
                                 unsigned join = FVM_STROKE_JOIN_ROUND);
/* Composite a source image through a path mask with the selected coverage policy. */
FVM_Result fvm_path_mask_quality(const FVM_Path *, FVM_RasterSurface *, const FVM_RasterSurface *,
                                 int even_odd, bool antialias);
/* Sample the existing gradient, then use the selected vector-edge coverage policy. */
FVM_Result fvm_path_draw_gradient_quality(const FVM_Path *, FVM_RasterSurface *,
                                          const FVM_Gradient *, int even_odd, bool antialias);
/* Select the spread mode and per-call edge coverage without changing global raster state. */
FVM_Result fvm_path_draw_gradient_spread_quality(const FVM_Path *, FVM_RasterSurface *,
                                                 const FVM_Gradient *, int even_odd,
                                                 unsigned int spread, bool antialias);
/* Stroke-gradient sampling uses a positive destination-space width and local edge policy. */
FVM_Result fvm_path_stroke_gradient_quality(const FVM_Path *, FVM_RasterSurface *,
                                            const FVM_Gradient *, double width,
                                            unsigned int spread, bool antialias);
/* Shared explicit color policy; width zero fills, positive destination width strokes. */
FVM_Result fvm_path_gradient_paint_quality(const FVM_Path *, FVM_RasterSurface *,
                                           const FVM_Gradient *, double width, int even_odd,
                                           unsigned int spread, unsigned int interpolation,
                                           bool antialias);
/* Explicit interior radial focal sampling; focal zero retains the legacy center path. */
FVM_Result fvm_path_gradient_focal_quality(const FVM_Path *, FVM_RasterSurface *,
                                           const FVM_Gradient *, double width, int even_odd,
                                           unsigned int spread, unsigned int interpolation,
                                           double focal, bool antialias,
                                           unsigned cap = FVM_STROKE_CAP_ROUND,
                                           unsigned join = FVM_STROKE_JOIN_ROUND);
/* A zero width selects fill coverage; a positive width selects backend stroke coverage. */
FVM_Result fvm_path_mask_width_quality(const FVM_Path *, FVM_RasterSurface *,
                                       const FVM_RasterSurface *, double width,
                                       int even_odd, bool antialias,
                                       unsigned cap = FVM_STROKE_CAP_ROUND,
                                       unsigned join = FVM_STROKE_JOIN_ROUND);
/* Sample existing bitmap paint without changing its smoothing option; select only edge AA. */
FVM_Result fvm_path_draw_bitmap_quality(const FVM_Path *, FVM_RasterSurface *,
                                        const FVM_BitmapPaint *, int even_odd, bool antialias);
/* Destination-space limit shared by bitmap stroke validation and its coverage preparation. */
#define FVM_BITMAP_STROKE_MAX_WIDTH 4096.0
/* Width zero fills; positive width applies the selected stroke mask to sampled pixels. */
FVM_Result fvm_path_bitmap_paint_quality(const FVM_Path *, FVM_RasterSurface *,
                                         const FVM_BitmapPaint *, double width,
                                         int even_odd, bool antialias,
                                         unsigned cap = FVM_STROKE_CAP_ROUND,
                                         unsigned join = FVM_STROKE_JOIN_ROUND);
#endif
