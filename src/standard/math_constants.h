/* FlashVM: Math namespace layout and loaded property attributes. */
#ifndef FVM_STANDARD_MATH_CONSTANTS_H
#define FVM_STANDARD_MATH_CONSTANTS_H /* Private Math metadata guard. */
#include "flashvm/property.h"
#define FVM_MATH_CONSTANT_COUNT 8U /* E, logarithms, PI and square-root constants. */
#define FVM_MATH_LOADED_PROPERTY_FLAGS (FVM_PROPERTY_BUILTIN_FLAGS | FVM_PROPERTY_READ_ONLY) /* Protected loaded methods/constants. */
#endif
