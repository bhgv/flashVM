/* FlashVM: traced live BitmapData fills for MovieClip drawing and portable bitmap sampling. */
#ifndef FVM_BITMAP_FILL_HPP
#define FVM_BITMAP_FILL_HPP /* Include guard for managed bitmap fill bridges. */
#include "../core/runtime.hpp"
#include "flashvm/bitmap_paint.h"
/* [GC:] Capture a live bitmap and copied matrix in an atomically published drawing descriptor. */
FVM_Result fvm_movieclip_bitmap_begin(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *,
                                      FVM_Value *, void *);
/* [GC:] Resolve retained bitmap pixels under the caller's native barrier and paint their current
 * contents. */
FVM_Result fvm_movieclip_bitmap_draw(FVM_Context *, FVM_Object *, const FVM_Affine &,
                                     const FVM_Path *, FVM_RasterSurface *, const double *,
                                     const double *, int, double stroke_width = 0);
#endif
