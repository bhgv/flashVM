/* FlashVM: required binary32 scene arithmetic model and integer-rounding constants. */
#ifndef FVM_MATRIX_CONSTANTS_H
#define FVM_MATRIX_CONSTANTS_H                    /* Scene matrix representation guard. */
#define FVM_MATRIX_FLOAT_RADIX 2                  /* Binary rather than decimal floating-point arithmetic. */
#define FVM_MATRIX_FLOAT_MANTISSA_BITS 24         /* IEEE binary32 includes its implicit leading bit. */
#define FVM_MATRIX_FLOAT_MAX_EXPONENT 128         /* Required binary32 exponent bound. */
#define FVM_MATRIX_FLOAT_BYTES 4U                 /* Supported native binary32 float storage size. */
#define FVM_MATRIX_BYTE_BITS 8                    /* Required byte width for the storage size check. */
#define FVM_MATRIX_HALF_UNIT 0.5                  /* Exact halfway case for nearest/even integer rounding. */
#define FVM_MATRIX_PARITY_MODULUS 2.0             /* Even lower integer wins a halfway tie. */
#define FVM_MATRIX_POSITIVE_OVERFLOW 2147483648.0 /* First positive value mapped to signed MIN. */
#define FVM_MATRIX_TWIPS_PER_PIXEL 20.0           /* Native scene coordinate conversion unit. */
#endif
