/* FlashVM: transactional BitmapData drawing from live pixels or normalized MovieClip subtrees. */
#ifndef FVM_BITMAP_DRAW_HPP
#define FVM_BITMAP_DRAW_HPP /* Include guard for bitmap drawing integration. */
#include "../core/runtime.hpp"
#include "flashvm/bitmap_paint.h"
/* [GC:] Draw a protected bitmap/clip into temporary pixels before publishing the blended result. */
FVM_Result fvm_bitmap_draw(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *,
                           void *);
/* [GC:] Render a retained clip subtree with an explicit root matrix/color and preserved child
 * poses; optional caller-owned coverage receives geometry independent of source alpha. */
FVM_Result fvm_movieclip_render_bitmap(FVM_Context *, FVM_Object *, FVM_RasterSurface *,
                                       const FVM_Affine &, const double *, const double *,
                                       FVM_RasterSurface *coverage = 0);
#endif
