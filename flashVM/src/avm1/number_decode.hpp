/* FlashVM: endian-independent arithmetic decoding of AVM1 IEEE numeric literal words. */
#ifndef FVM_AVM1_NUMBER_DECODE_HPP
#define FVM_AVM1_NUMBER_DECODE_HPP /* Shared interpreter/compiler numeric-literal guard. */
#include "../core/numeric_constants.h"
#include <cmath>
#include <limits>


/* Decode binary32 bits without alignment casts or depending on native float storage. */
inline double fvm_avm1_decode_float(unsigned long bits)
{
    unsigned int exponent = (unsigned int)((bits >> FVM_FLOAT_FRACTION_BITS) & FVM_FLOAT_EXPONENT_MASK);
    unsigned long fraction = bits & FVM_FLOAT_FRACTION_MASK;
    double number;
    if (exponent == FVM_FLOAT_EXPONENT_MASK)
    {
        number = fraction ? std::numeric_limits<double>::quiet_NaN() : std::numeric_limits<double>::infinity();
    }
    else
    {
        number = std::ldexp((double)(fraction + (exponent ? FVM_FLOAT_IMPLICIT_ONE : 0)),
                            exponent ? (int)exponent - FVM_FLOAT_NORMAL_SCALE_BIAS : -FVM_FLOAT_SUBNORMAL_SCALE_BIAS);
    }
    return (bits & FVM_UINT32_SIGN_BIT) ? -number : number;
}


/* Decode AVM1's high-word-first binary64 layout after each word's little-endian read. */
inline double fvm_avm1_decode_double(unsigned long high, unsigned long low)
{
    unsigned int exponent = (unsigned int)((high >> FVM_DOUBLE_HIGH_FRACTION_BITS) & FVM_DOUBLE_EXPONENT_MASK);
    double fraction = (double)(high & FVM_DOUBLE_HIGH_FRACTION_MASK) * FVM_UINT32_MODULUS + (double)low;
    double number;
    if (exponent == FVM_DOUBLE_EXPONENT_MASK)
    {
        number = fraction ? std::numeric_limits<double>::quiet_NaN() : std::numeric_limits<double>::infinity();
    }
    else
    {
        number = std::ldexp(fraction + (exponent ? FVM_DOUBLE_IMPLICIT_ONE : 0.0),
                            exponent ? (int)exponent - FVM_DOUBLE_NORMAL_SCALE_BIAS : -FVM_DOUBLE_SUBNORMAL_SCALE_BIAS);
    }
    return (high & FVM_UINT32_SIGN_BIT) ? -number : number;
}
#endif
