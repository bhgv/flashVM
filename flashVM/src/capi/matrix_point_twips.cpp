/* FlashVM: binary32 scene point application shared by C, generated code and loaded AVM1. */
#include "flashvm/matrix.h"
#include "../core/fixed32.hpp"
#include "../core/matrix_float.hpp"
/* Force binary32 rounding between products and addition, without fused/extended intermediates. */
extern "C" FVM_Result fvm_matrix_point_twips(const FVM_TwipMatrix *matrix, long x, long y,
                                             long *out_x, long *out_y)
{
    if (!matrix || !out_x || !out_y || out_x == out_y || !fvm_fixed_detail::valid(x) ||
        !fvm_fixed_detail::valid(y) || !fvm_fixed_detail::valid(matrix->tx) ||
        !fvm_fixed_detail::valid(matrix->ty))
        return FVM_INVALID_ARGUMENT;
    if (!fvm_matrix_detail::supported())
        return FVM_UNSUPPORTED;
    volatile float fx = (float)x, fy = (float)y;
    volatile float ax = matrix->a * fx, cy = matrix->c * fy;
    volatile float bx = matrix->b * fx, dy = matrix->d * fy;
    volatile float linear_x = ax + cy, linear_y = bx + dy;
    long result_x = fvm_fixed_detail::signed_word(
        (unsigned long)fvm_matrix_detail::rounded_twip(linear_x) + (unsigned long)matrix->tx);
    long result_y = fvm_fixed_detail::signed_word(
        (unsigned long)fvm_matrix_detail::rounded_twip(linear_y) + (unsigned long)matrix->ty);
    *out_x = result_x;
    *out_y = result_y;
    return FVM_OK;
}
