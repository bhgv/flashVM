/* FlashVM: context-free C89 signed 16.16 matrix and signed-twip arithmetic. */
#ifndef FLASHVM_FIXED_H
#define FLASHVM_FIXED_H /* Public fixed matrix arithmetic guard. */
#include "flashvm.h"

/* Raw signed 32-bit representation bounds, also on hosts with wider long. */
#define FVM_FIXED32_MAX 2147483647L
#define FVM_FIXED32_MIN (-2147483647L - 1L)
#define FVM_FIXED16_FRACTION_BITS 16U /* Low coefficient bits represent the fractional part. */
#define FVM_FIXED16_ONE 65536L        /* Identity coefficient in signed 16.16 representation. */
#ifdef __cplusplus
extern "C"
{
#endif
/* A point is mapped to (a*x+c*y+tx, b*x+d*y+ty), with each product shifted separately. */
typedef struct FVM_FixedMatrix
{
    long a;  /* Raw signed 16.16 horizontal coefficient for input x. */
    long b;  /* Raw signed 16.16 vertical coefficient for input x. */
    long c;  /* Raw signed 16.16 horizontal coefficient for input y. */
    long d;  /* Raw signed 16.16 vertical coefficient for input y. */
    long tx; /* Signed 32-bit horizontal translation in twips. */
    long ty; /* Signed 32-bit vertical translation in twips. */
} FVM_FixedMatrix;


/* Shift each signed product down by 16 bits (negative fractions round toward minus infinity),
     * then add translation, wrapping every result to signed 32 bits. Inputs must be in the
     * signed 32-bit range. Outputs must be nonnull and distinct; they may alias matrix fields.
     * Invalid inputs preserve outputs. No context, allocation, callbacks or GC are involved. */
FVM_API FVM_Result fvm_fixed_matrix_point(const FVM_FixedMatrix *, long x, long y, long *out_x,
                                          long *out_y);


/* Compose left * right, so right is applied first. Every scalar product is shifted before
     * addition; results wrap to signed 32 bits. All six fields must fit the signed 32-bit range.
     * Output may alias either input. Invalid inputs preserve output; no allocation/callbacks. */
FVM_API FVM_Result fvm_fixed_matrix_multiply(const FVM_FixedMatrix *left,
                                             const FVM_FixedMatrix *right,
                                             FVM_FixedMatrix *out);
#ifdef __cplusplus
}
#endif
#endif
