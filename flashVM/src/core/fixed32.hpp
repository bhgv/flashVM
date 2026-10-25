/* FlashVM: exact modular fixed-point helpers using unsigned limbs, without 64-bit integers. */
#ifndef FVM_FIXED32_HPP
#define FVM_FIXED32_HPP /* Private fixed-point helper guard. */
#include "flashvm/fixed.h"
#include "fixed32_constants.h"

namespace fvm_fixed_detail
{


/* Reject out-of-representation inputs before arithmetic on 32-bit or wider long hosts. */
inline bool valid(long value)
{
    return value >= FVM_FIXED32_MIN && value <= FVM_FIXED32_MAX;
}


/* Validate the complete input matrix before any aliased output can be modified. */
inline bool valid(const FVM_FixedMatrix *matrix)
{
    return matrix && valid(matrix->a) && valid(matrix->b) && valid(matrix->c) && valid(matrix->d) &&
           valid(matrix->tx) && valid(matrix->ty);
}


/* Decode a low 32-bit word without unsigned-to-signed casts outside long's positive range. */
inline long signed_word(unsigned long bits)
{
    bits &= FVM_FIXED32_WORD_MASK;
    return bits <= (unsigned long)FVM_FIXED32_MAX ? (long)bits : -1L - (long)(FVM_FIXED32_WORD_MASK - bits);
}


/* Compute low32(floor(a*b/65536)) using signed-high/unsigned-low limb decomposition.
 * Word must be an unsigned type of at least 32 bits. Testing both uint and ulong exercises
 * native 32-bit modular intermediates even on a host where long is wider. */
template <class Word> inline unsigned long product_word(long a, long b)
{
    const Word mask = (Word)FVM_FIXED32_WORD_MASK;
    const Word limb_mask = (Word)FVM_FIXED32_LIMB_MASK;
    Word left = (Word)a & mask, right = (Word)b & mask;
    Word low_left = left & limb_mask, low_right = right & limb_mask;
    Word high_left = left >> FVM_FIXED16_FRACTION_BITS;
    Word high_right = right >> FVM_FIXED16_FRACTION_BITS;
    if (high_left & (Word)FVM_FIXED32_LIMB_SIGN)
    {
        high_left |= (Word)FVM_FIXED32_EXTEND_MASK;
    }
    if (high_right & (Word)FVM_FIXED32_LIMB_SIGN)
    {
        high_right |= (Word)FVM_FIXED32_EXTEND_MASK;
    }
    Word result = (low_left * low_right) >> FVM_FIXED16_FRACTION_BITS;
    result += high_left * low_right;
    result += high_right * low_left;
    result += ((high_left * high_right) & limb_mask) << FVM_FIXED16_FRACTION_BITS;
    return (unsigned long)(result & mask);
}


/* Apply one matrix row with separately truncated products and modular translation addition. */
inline long row(long a, long b, long x, long y, long translation)
{
    unsigned long result = product_word<unsigned long>(a, x);
    result += product_word<unsigned long>(b, y);
    result += (unsigned long)translation;
    return signed_word(result);
}
} // namespace fvm_fixed_detail
#endif
