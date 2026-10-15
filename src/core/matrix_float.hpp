/* FlashVM: shared binary32 model validation and signed-twip rounding for scene services. */
#ifndef FVM_MATRIX_FLOAT_HPP
#define FVM_MATRIX_FLOAT_HPP /* Scene floating helper guard. */
#include "flashvm/matrix.h"
#include "matrix_constants.h"
#include <cfloat>
#include <climits>
#include <cmath>
namespace fvm_matrix_detail
{
/* Require the native binary32 model before committing scene arithmetic outputs. */
inline bool supported()
{
    return FLT_RADIX == FVM_MATRIX_FLOAT_RADIX && FLT_MANT_DIG == FVM_MATRIX_FLOAT_MANTISSA_BITS &&
           FLT_MAX_EXP == FVM_MATRIX_FLOAT_MAX_EXPONENT &&
           sizeof(float) == FVM_MATRIX_FLOAT_BYTES && CHAR_BIT == FVM_MATRIX_BYTE_BITS;
}
/* Round a binary32 term without unsafe floating-to-integer casts or C99 rounding APIs. */
inline long rounded_twip(float value)
{
    if (!(value >= -FLT_MAX && value <= FLT_MAX))
        return 0;
    double number = value;
    if (number >= FVM_MATRIX_POSITIVE_OVERFLOW || number <= (double)FVM_FIXED32_MIN)
        return FVM_FIXED32_MIN;
    double lower = std::floor(number), fraction = number - lower;
    if (fraction > FVM_MATRIX_HALF_UNIT ||
        (fraction == FVM_MATRIX_HALF_UNIT && std::fmod(lower, FVM_MATRIX_PARITY_MODULUS) != 0))
        lower += 1;
    return (long)lower;
}
} // namespace fvm_matrix_detail
#endif
