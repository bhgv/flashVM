/* FlashVM: bounded logical BitmapData pixel operations independent of graphics backends. */
#ifndef FVM_BITMAP_OPS_HPP
#define FVM_BITMAP_OPS_HPP /* Include guard for logical bitmap operations. */
#include "../core/runtime.hpp"


/* [GC:] Fill one four-connected pixel component in temporary storage before atomic publication. */
FVM_Result fvm_bitmap_flood_fill(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* [GC:] Return a typed Rectangle enclosing matching or nonmatching masked ARGB pixels. */
FVM_Result fvm_bitmap_color_bounds(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* [GC:] Replace matching masked pixels or copy unmatched source pixels from a clipped snapshot. */
FVM_Result fvm_bitmap_threshold(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* [GC:] Merge a copied source region with independently bounded per-channel destination weights. */
FVM_Result fvm_bitmap_merge(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* [GC:] Remap copied source channels through cached public palette entries before pixel
 * publication. */
FVM_Result fvm_bitmap_palette_map(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* [GC:] Test copied public geometry against live bitmap alpha without borrowing across callbacks.
 */
FVM_Result fvm_bitmap_hit_test(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);


/* [GC:] Stage deterministic channel noise before replacing the receiver's complete pixel buffer. */
FVM_Result fvm_bitmap_noise(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *);
#endif
