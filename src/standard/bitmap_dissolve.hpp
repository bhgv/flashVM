/* FlashVM: staged deterministic BitmapData dissolve with portable continuation cursors. */
#ifndef FVM_BITMAP_DISSOLVE_HPP
#define FVM_BITMAP_DISSOLVE_HPP /* Include guard for pixel dissolve integration. */
#include "../core/runtime.hpp"
/* [GC:] Coerce protected public arguments, then publish selected copied/fill pixels atomically. */
FVM_Result fvm_bitmap_pixel_dissolve(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *,
                                     FVM_Value *, void *);
#endif
