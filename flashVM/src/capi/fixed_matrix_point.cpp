/* FlashVM: context-free fixed-point projection for C callers and generated code. */
#include "../core/fixed32.hpp"
/* Validate first, compute both coordinates, then publish without allocating or invoking script. */
extern "C" FVM_Result fvm_fixed_matrix_point(const FVM_FixedMatrix *matrix, long x, long y,
                                             long *out_x, long *out_y)
{
    if (!out_x || !out_y || out_x == out_y || !fvm_fixed_detail::valid(matrix) ||
        !fvm_fixed_detail::valid(x) || !fvm_fixed_detail::valid(y))
        return FVM_INVALID_ARGUMENT;
    long result_x = fvm_fixed_detail::row(matrix->a, matrix->c, x, y, matrix->tx);
    long result_y = fvm_fixed_detail::row(matrix->b, matrix->d, x, y, matrix->ty);
    *out_x = result_x;
    *out_y = result_y;
    return FVM_OK;
}
