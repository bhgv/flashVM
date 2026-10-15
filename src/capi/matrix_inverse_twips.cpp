/* FlashVM: binary32 scene inverse with rounded translation, independent of VM execution state. */
#include "flashvm/matrix.h"
#include "../core/fixed32.hpp"
#include "../core/matrix_float.hpp"
/* Preserve output on singular/invalid input, and stage every field before aliased publication. */
extern "C" FVM_Result fvm_matrix_inverse_twips(const FVM_TwipMatrix *matrix, FVM_TwipMatrix *out)
{
    if (!matrix || !out || !fvm_fixed_detail::valid(matrix->tx) ||
        !fvm_fixed_detail::valid(matrix->ty))
        return FVM_INVALID_ARGUMENT;
    if (!fvm_matrix_detail::supported())
        return FVM_UNSUPPORTED;
    volatile float ad = matrix->a * matrix->d, bc = matrix->b * matrix->c;
    volatile float determinant = ad - bc;
    if (!(std::fabs((double)determinant) > FLT_EPSILON))
        return FVM_INVALID_ARGUMENT;
    FVM_TwipMatrix result;
    volatile float a = matrix->d / determinant, b = matrix->b / -determinant;
    volatile float c = matrix->c / -determinant, d = matrix->a / determinant;
    result.a = a;
    result.b = b;
    result.c = c;
    result.d = d;
    volatile float tx = (float)matrix->tx, ty = (float)matrix->ty;
    volatile float dx = matrix->d * tx, cy = matrix->c * ty;
    volatile float bx = matrix->b * tx, ay = matrix->a * ty;
    volatile float x_difference = dx - cy, y_difference = bx - ay;
    volatile float inverse_x = x_difference / -determinant;
    volatile float inverse_y = y_difference / determinant;
    result.tx = fvm_matrix_detail::rounded_twip(inverse_x);
    result.ty = fvm_matrix_detail::rounded_twip(inverse_y);
    *out = result;
    return FVM_OK;
}
