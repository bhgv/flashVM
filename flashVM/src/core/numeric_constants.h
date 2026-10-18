/* FlashVM: portable integer, floating-point and bounded-runtime constants. */
#ifndef FVM_NUMERIC_CONSTANTS_H
#define FVM_NUMERIC_CONSTANTS_H /* Header guard. */
/* Mask retaining the low 32 unsigned bits. */
#define FVM_UINT32_MASK 0xffffffffUL
/* Bit identifying the negative half of a signed 32-bit value. */
#define FVM_UINT32_SIGN_BIT 0x80000000UL
/* Magnitude bits below the signed 32-bit sign bit. */
#define FVM_UINT32_MAGNITUDE_MASK 0x7fffffffUL
/* Double-exact modulus 2^32 used by wrapping numeric conversion. */
#define FVM_UINT32_MODULUS 4294967296.0
/* Largest unsigned 32-bit value as a Number. */
#define FVM_UINT32_MAX_NUMBER 4294967295.0
/* Lowest signed 32-bit value representable by the integer fast path. */
#define FVM_INT32_MIN_NUMBER (-2147483648.0)
/* Highest signed 32-bit value representable by the integer fast path. */
#define FVM_INT32_MAX_NUMBER 2147483647.0
/* Signed-long positive half used to decode Int32 without overflowing casts. */
#define FVM_INT32_MAX_LONG 2147483647L
/* Width of AVM1 integer bit operations, independent of host long width. */
#define FVM_UINT32_BIT_COUNT 32U
/* Mask implementing a 0..31 shift count. */
#define FVM_UINT32_SHIFT_MASK 31U
/* Mask retaining one UTF-16 code unit. */
#define FVM_UTF16_UNIT_MASK 0xffffUL
/* IEEE binary32 stored fraction bits. */
#define FVM_FLOAT_FRACTION_MASK 0x7fffffUL
/* IEEE binary32 implicit leading significand unit. */
#define FVM_FLOAT_IMPLICIT_ONE 0x800000UL
/* Number of stored binary32 fraction bits. */
#define FVM_FLOAT_FRACTION_BITS 23U
/* IEEE binary32 all-ones exponent, selecting infinity/NaN. */
#define FVM_FLOAT_EXPONENT_MASK 255U
/* Binary32 exponent bias plus stored fraction width. */
#define FVM_FLOAT_NORMAL_SCALE_BIAS 150
/* Binary32 zero-exponent fraction scaling bias. */
#define FVM_FLOAT_SUBNORMAL_SCALE_BIAS 149
/* Stored fraction bits in the high binary64 word. */
#define FVM_DOUBLE_HIGH_FRACTION_MASK 0xfffffUL
/* High-word binary64 fraction width before exponent bits. */
#define FVM_DOUBLE_HIGH_FRACTION_BITS 20U
/* IEEE binary64 all-ones exponent, selecting infinity/NaN. */
#define FVM_DOUBLE_EXPONENT_MASK 2047U
/* Double-exact implicit leading significand unit 2^52. */
#define FVM_DOUBLE_IMPLICIT_ONE 4503599627370496.0
/* Binary64 exponent bias plus stored fraction width. */
#define FVM_DOUBLE_NORMAL_SCALE_BIAS 1075
/* Binary64 zero-exponent fraction scaling bias. */
#define FVM_DOUBLE_SUBNORMAL_SCALE_BIAS 1074
/* Bound on prototype/captured-scope lookup and cycle validation. */
#define FVM_PROTOTYPE_MAX_DEPTH 1024U
/* Significant decimal digits retained by loaded AVM1 spelling. */
#define FVM_AVM1_DECIMAL_DIGITS 15
/* Largest base-10 exponent using fixed AVM1 notation. */
#define FVM_AVM1_FIXED_EXPONENT_MAX 14
/* Smallest base-10 exponent using fixed AVM1 notation. */
#define FVM_AVM1_FIXED_EXPONENT_MIN (-5)
/* Historical precision of log10(2) used by AVM1 decimal exponent estimation. */
#define FVM_AVM1_LOG10_2 0.301029995663981
#endif
