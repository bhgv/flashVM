/* FlashVM: safe native pixel/coefficient preparation for binary32 scene services. */
#ifndef FVM_SCENE_NUMERIC_HPP
#define FVM_SCENE_NUMERIC_HPP /* Native scene number conversion guard. */
#include "flashvm/matrix.h"
#include "matrix_constants.h"
#include <cmath>
#include <limits>
namespace fvm_scene_detail
{
/* Clamp before multiplying, truncate toward zero, and convert only bounded signed32 integers. */
inline long input_twips(double pixels)
{
    if (pixels != pixels)
        return 0;
    if (pixels >= (double)FVM_FIXED32_MAX / FVM_MATRIX_TWIPS_PER_PIXEL)
        return FVM_FIXED32_MAX;
    if (pixels <= (double)FVM_FIXED32_MIN / FVM_MATRIX_TWIPS_PER_PIXEL)
        return FVM_FIXED32_MIN;
    double twips = pixels * FVM_MATRIX_TWIPS_PER_PIXEL;
    if (twips >= (double)FVM_FIXED32_MAX)
        return FVM_FIXED32_MAX;
    if (twips <= (double)FVM_FIXED32_MIN)
        return FVM_FIXED32_MIN;
    return (long)(twips < 0 ? std::ceil(twips) : std::floor(twips));
}
/* Convert double coefficients without out-of-range floating conversions. */
inline float coefficient(double value)
{
    if (value != value)
        return std::numeric_limits<float>::quiet_NaN();
    double limit = std::numeric_limits<float>::max();
    if (value > limit)
        return std::numeric_limits<float>::infinity();
    if (value < -limit)
        return -std::numeric_limits<float>::infinity();
    return (float)value;
}
} // namespace fvm_scene_detail
#endif
