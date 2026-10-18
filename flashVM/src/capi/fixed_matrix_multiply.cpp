/* FlashVM: context-free signed 16.16 matrix composition for C callers and generated code. */
#include "../core/fixed32.hpp"
/* Stage the full product so output can alias either validated input matrix. */
extern "C" FVM_Result fvm_fixed_matrix_multiply(const FVM_FixedMatrix *left,
                                                const FVM_FixedMatrix *right, FVM_FixedMatrix *out)
{
    if (!out || !fvm_fixed_detail::valid(left) || !fvm_fixed_detail::valid(right))
        return FVM_INVALID_ARGUMENT;
    FVM_FixedMatrix result;
    result.a = fvm_fixed_detail::row(left->a, left->c, right->a, right->b, 0);
    result.b = fvm_fixed_detail::row(left->b, left->d, right->a, right->b, 0);
    result.c = fvm_fixed_detail::row(left->a, left->c, right->c, right->d, 0);
    result.d = fvm_fixed_detail::row(left->b, left->d, right->c, right->d, 0);
    result.tx = fvm_fixed_detail::row(left->a, left->c, right->tx, right->ty, left->tx);
    result.ty = fvm_fixed_detail::row(left->b, left->d, right->tx, right->ty, left->ty);
    *out = result;
    return FVM_OK;
}
