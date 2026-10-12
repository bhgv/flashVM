/* FlashVM: managed gradient-paint decoding for MovieClip vector rendering. */
#ifndef FVM_STANDARD_GRADIENT_HPP
#define FVM_STANDARD_GRADIENT_HPP /* Include guard for gradient graph rendering. */
#include "../core/runtime.hpp"
#include "flashvm/gradient.h"

/* [GC:] Read immutable managed gradient data and draw with its composed matrix/color transform. */
FVM_Result fvm_movieclip_gradient_draw(FVM_Context *, FVM_Object *paint, const FVM_Affine &world,
                                       const FVM_Path *, FVM_RasterSurface *,
                                       const double multiply[4], const double add[4], int even_odd);
/* Copy ActionScript gradient arguments into managed drawing state without borrowing arrays. */
FVM_Result fvm_movieclip_gradient_begin(FVM_Context *, FVM_Object *, unsigned int,
                                        const FVM_Value *, FVM_Value *, void *);
#endif
