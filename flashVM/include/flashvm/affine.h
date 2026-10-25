/* FlashVM: context-free double affine projection for hosts and generated code. */
#ifndef FLASHVM_AFFINE_H
#define FLASHVM_AFFINE_H /* Public double projection interface guard. */
#include "path.h"

/* Project a position, subtracting the affine translation before inversion. */
#define FVM_AFFINE_POINT 0
/* Project a displacement, ignoring the affine translation. */
#define FVM_AFFINE_VECTOR 1
#ifdef __cplusplus
extern "C"
{
#endif


/* Apply the inverse linear transform to a point or vector using double arithmetic.
     * A zero/nonfinite determinant or nonfinite result returns INVALID_ARGUMENT.
     * Mode must be POINT or VECTOR. Translation is not read in VECTOR mode.
     * Outputs must be non-null and distinct; they may alias matrix fields.
     * All errors preserve both outputs. No context, allocation, callbacks or GC.
     * This host policy does not implement loaded SWF binary32/twip rounding. */
FVM_API FVM_Result fvm_affine_inverse_point(const FVM_Affine *matrix, int mode, double x,
                                            double y, double *out_x, double *out_y);


/* Apply the double affine expression without finite-result validation or twip rounding.
     * NaN/Infinity propagate according to ordinary floating-point arithmetic. Outputs must
     * be non-null/distinct and may alias matrix fields. Invalid pointers preserve outputs.
     * No context, allocation, callbacks or GC. */
FVM_API FVM_Result fvm_affine_transform_point(const FVM_Affine *matrix, double x, double y,
                                              double *out_x, double *out_y);


/* Construct the inverse double matrix, then its inverse translation. Zero/NaN
     * determinants are invalid and preserve output; other IEEE results propagate.
     * This deliberately differs from strict inverse_point, including operation order.
     * Output may alias input. No context, allocation, callbacks or GC. */
FVM_API FVM_Result fvm_affine_invert(const FVM_Affine *matrix, FVM_Affine *out);
#ifdef __cplusplus
}
#endif
#endif
