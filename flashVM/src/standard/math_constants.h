/* FlashVM: Math namespace layout and loaded property attributes. */
#ifndef FVM_STANDARD_MATH_CONSTANTS_H
#define FVM_STANDARD_MATH_CONSTANTS_H /* Private Math metadata guard. */
#include "flashvm/property.h"

#define FVM_MATH_CONSTANT_COUNT 8U                        /* E, logarithms, PI and square-root constants. */
#define FVM_MATH_LOADED_PROPERTY_FLAGS                                                                                 \
    (FVM_PROPERTY_BUILTIN_FLAGS | FVM_PROPERTY_READ_ONLY) /* Protected loaded methods/constants. */
/* Correctly rounded binary64 values; independent of host libm evaluation. */
#define FVM_MATH_CONSTANT_E 2.71828182845904523536028747135266250        /* Euler number. */
#define FVM_MATH_CONSTANT_LN10 2.30258509299404568401799145468436421     /* Natural logarithm of ten. */
#define FVM_MATH_CONSTANT_LN2 0.693147180559945309417232121458176568     /* Natural logarithm of two. */
#define FVM_MATH_CONSTANT_LOG10E 0.434294481903251827651128918916605082  /* Base-ten logarithm of Euler number. */
#define FVM_MATH_CONSTANT_LOG2E 1.44269504088896340735992468100189214    /* Base-two logarithm of Euler number. */
#define FVM_MATH_CONSTANT_PI 3.14159265358979323846264338327950288       /* Circle circumference-to-diameter ratio. */
#define FVM_MATH_CONSTANT_SQRT1_2 0.707106781186547524400844362104849039 /* Square root of one half. */
#define FVM_MATH_CONSTANT_SQRT2 1.41421356237309504880168872420969808    /* Square root of two. */
#endif
