/* FlashVM: deterministic staged Perlin turbulence for ActionScript BitmapData. */
#ifndef FVM_BITMAP_PERLIN_HPP
#define FVM_BITMAP_PERLIN_HPP /* Include guard for bitmap Perlin operations. */
#include "../core/runtime.hpp"
/* [GC:] Read public octave offsets before building and atomically publishing a noise buffer. */
FVM_Result fvm_bitmap_perlin_noise(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *,
                                   FVM_Value *, void *);
#endif
