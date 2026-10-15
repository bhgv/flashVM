/* FlashVM: context-free binary32 scene matrix composition for C and generated code. */
#include "flashvm/matrix.h"
#include "../core/fixed32.hpp"
#include "../core/matrix_float.hpp"
/* Stage coefficients and rounded translations before any possibly aliased output publication. */
extern "C" FVM_Result fvm_matrix_multiply_twips(const FVM_TwipMatrix *left,
                                                const FVM_TwipMatrix *right, FVM_TwipMatrix *out)
{
    if (!left || !right || !out || !fvm_fixed_detail::valid(left->tx) ||
        !fvm_fixed_detail::valid(left->ty) || !fvm_fixed_detail::valid(right->tx) ||
        !fvm_fixed_detail::valid(right->ty))
        return FVM_INVALID_ARGUMENT;
    if (!fvm_matrix_detail::supported())
        return FVM_UNSUPPORTED;
    FVM_TwipMatrix result;
    result.a = fvm_matrix_detail::linear(left->a, left->c, right->a, right->b);
    result.b = fvm_matrix_detail::linear(left->b, left->d, right->a, right->b);
    result.c = fvm_matrix_detail::linear(left->a, left->c, right->c, right->d);
    result.d = fvm_matrix_detail::linear(left->b, left->d, right->c, right->d);
    volatile float tx = (float)right->tx, ty = (float)right->ty;
    result.tx =
        fvm_fixed_detail::signed_word((unsigned long)fvm_matrix_detail::rounded_twip(
                                          fvm_matrix_detail::linear(left->a, left->c, tx, ty)) +
                                      (unsigned long)left->tx);
    result.ty =
        fvm_fixed_detail::signed_word((unsigned long)fvm_matrix_detail::rounded_twip(
                                          fvm_matrix_detail::linear(left->b, left->d, tx, ty)) +
                                      (unsigned long)left->ty);
    *out = result;
    return FVM_OK;
}
