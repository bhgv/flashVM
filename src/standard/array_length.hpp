/* FlashVM: allocation-free AVM1 Int32 interpretation of a stored numeric Array length. */
#ifndef FVM_ARRAY_LENGTH_HPP
#define FVM_ARRAY_LENGTH_HPP /* Guard for the numeric length conversion helper. */
#include "../core/numeric_constants.h"
#include <cmath>
/* Interpret Number through truncation/modulo, without changing its retained raw value. */
inline long fvm_array_number_length(double value)
{
    if (value != value || !value || std::fabs(value) == HUGE_VAL)
        return 0;
    const double whole = value < 0 ? std::ceil(value) : std::floor(value);
    double wrapped = std::fmod(whole, FVM_UINT32_MODULUS);
    if (wrapped < 0)
        wrapped += FVM_UINT32_MODULUS;
    return (long)(wrapped > FVM_INT32_MAX_NUMBER ? wrapped - FVM_UINT32_MODULUS : wrapped);
}
#endif
