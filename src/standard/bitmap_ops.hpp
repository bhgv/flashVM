/* FlashVM: bounded logical BitmapData pixel operations independent of graphics backends. */
#ifndef FVM_BITMAP_OPS_HPP
#define FVM_BITMAP_OPS_HPP /* Include guard for logical bitmap operations. */
#include "../core/runtime.hpp"
/* [GC:] Fill one four-connected pixel component in temporary storage before atomic publication. */
FVM_Result fvm_bitmap_flood_fill(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *,
                                 FVM_Value *, void *);
/* [GC:] Return a typed Rectangle enclosing matching or nonmatching masked ARGB pixels. */
FVM_Result fvm_bitmap_color_bounds(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *,
                                   FVM_Value *, void *);
/* [GC:] Replace matching masked pixels or copy unmatched source pixels from a clipped snapshot. */
FVM_Result fvm_bitmap_threshold(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *,
                                FVM_Value *, void *);
#endif
