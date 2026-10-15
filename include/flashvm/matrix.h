/* FlashVM: context-free binary32 scene matrix application to signed integer twips. */
#ifndef FLASHVM_MATRIX_H
#define FLASHVM_MATRIX_H /* Public scene matrix arithmetic guard. */
#include "fixed.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Scene representation differs from raw SWF signed 16.16 coefficient storage. */
    typedef struct FVM_TwipMatrix
    {
        float a; /* Binary32 horizontal coefficient for point x. */
        float b; /* Binary32 vertical coefficient for point x. */
        float c; /* Binary32 horizontal coefficient for point y. */
        float d; /* Binary32 vertical coefficient for point y. */
        long tx; /* Signed 32-bit horizontal translation in twips. */
        long ty; /* Signed 32-bit vertical translation in twips. */
    } FVM_TwipMatrix;
    /* Evaluate separate binary32 products and sums, round linear terms to nearest/even integer,
     * then add translations modulo 2^32. Nonfinite linear terms become zero; positive overflow
     * and negative underflow become signed MIN. Inputs x/y and translations must fit signed32.
     * Outputs must be distinct/non-null and may alias translation fields; errors preserve them.
     * No context, allocation, callbacks or GC. Requires binary32 float and default FP environment;
     * unsupported float formats return FVM_UNSUPPORTED. Matrix storage is native, not a wire ABI.
     */
    FVM_API FVM_Result fvm_matrix_point_twips(const FVM_TwipMatrix *, long x, long y, long *out_x,
                                              long *out_y);
    /* Invert using separately rounded binary32 operations and nearest/even translation twips.
     * abs(determinant) must exceed binary32 epsilon; otherwise INVALID_ARGUMENT preserves output.
     * Input translations must fit signed32. Output may alias input. No context/allocation/GC;
     * requires the same binary32 format and FP environment as point application. */
    FVM_API FVM_Result fvm_matrix_inverse_twips(const FVM_TwipMatrix *, FVM_TwipMatrix *out);
#ifdef __cplusplus
}
#endif
#endif
