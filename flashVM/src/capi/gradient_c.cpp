/* FlashVM: backend-independent gradient sampling composed through the existing vector mask API. */
#include "../render/quality_internal.hpp"

#include "flashvm/gradient.h"
#include "../details/vector.hpp"
#include <cmath>
#include <climits>
#include <cstring>
#include <new>

namespace
{
/* Reject NaN/infinity and bound user-supplied matrix/color arithmetic before sampling. */
bool bounded(double n, double limit)
{
    return n >= -limit && n <= limit;
}

#if defined(FVM_HAS_AGG) || defined(FVM_HAS_CAIRO)
/* Clamp a final straight color component after the complete inherited color transform. */
unsigned char component(double value)
{
    if (!(value > 0))
        return 0;
    if (value >= 255)
        return 255;
    return (unsigned char)(value + 0.5);
}

/* Interpolate one ramp sample; the last duplicate stop wins exactly at its ratio. */
FVM_RasterColor sample(const FVM_Gradient &gradient, double ratio)
{
    size_t left = 0;
    while (left + 1 < gradient.count && gradient.stops[left + 1].ratio <= ratio)
        ++left;
    size_t right = left;
    double weight = 0;
    if (ratio > gradient.stops[left].ratio && left + 1 < gradient.count)
    {
        right = left + 1;
        weight = (ratio - gradient.stops[left].ratio) /
                 (gradient.stops[right].ratio - gradient.stops[left].ratio);
    }
    const FVM_RasterColor &a = gradient.stops[left].color;
    const FVM_RasterColor &b = gradient.stops[right].color;
    const double av[] = {(double)a.r, (double)a.g, (double)a.b, (double)a.a};
    const double bv[] = {(double)b.r, (double)b.g, (double)b.b, (double)b.a};
    unsigned char values[4];
    for (unsigned int i = 0; i < 4; ++i)
        values[i] =
            component((av[i] + (bv[i] - av[i]) * weight) * gradient.multiply[i] + gradient.add[i]);
    FVM_RasterColor out = {values[0], values[1], values[2], values[3]};
    return out;
}

#endif

/* Validate all borrowed inputs and construct a bounded inverse gradient transform. */
bool validate(const FVM_Path *path, const FVM_RasterSurface *surface, const FVM_Gradient *gradient,
              int even_odd, FVM_Affine &inverse)
{
    if (!path || !surface || !surface->pixels || !surface->width || !surface->height ||
        surface->width > 4096 || surface->height > 4096 ||
        surface->stride < (size_t)surface->width * 4 ||
        surface->stride > (size_t)INT_MAX / surface->height || !gradient ||
        (gradient->type != FVM_GRADIENT_LINEAR && gradient->type != FVM_GRADIENT_RADIAL) ||
        !gradient->stops || !gradient->count || gradient->count > FVM_GRADIENT_MAX_STOPS ||
        (even_odd != 0 && even_odd != 1))
        return false;
    for (size_t i = 0; i < fvm_path_count(path); ++i)
    {
        FVM_PathCommand command;
        if (fvm_path_get(path, i, &command) != FVM_OK)
            return false;
        unsigned int points = command.verb == FVM_PATH_CLOSE       ? 0
                              : command.verb == FVM_PATH_CUBIC     ? 3
                              : command.verb == FVM_PATH_QUADRATIC ? 2
                                                                   : 1;
        for (unsigned int j = 0; j < points; ++j)
            if (!bounded(command.x[j], 4096) || !bounded(command.y[j], 4096))
                return false;
    }
    for (size_t i = 0; i < gradient->count; ++i)
        if (!(gradient->stops[i].ratio >= 0 && gradient->stops[i].ratio <= 1) ||
            (i && gradient->stops[i].ratio < gradient->stops[i - 1].ratio))
            return false;
    for (unsigned int i = 0; i < 4; ++i)
        if (!bounded(gradient->multiply[i], 65536) || !bounded(gradient->add[i], 65536))
            return false;
    const FVM_Affine &m = gradient->matrix;
    if (!bounded(m.xx, 1000000) || !bounded(m.xy, 1000000) || !bounded(m.yx, 1000000) ||
        !bounded(m.yy, 1000000) || !bounded(m.x0, 1000000) || !bounded(m.y0, 1000000))
        return false;
    double determinant = m.xx * m.yy - m.xy * m.yx;
    if (!(std::fabs(determinant) >= 1e-20))
        return false;
    inverse.xx = m.yy / determinant;
    inverse.xy = -m.xy / determinant;
    inverse.yx = -m.yx / determinant;
    inverse.yy = m.xx / determinant;
    inverse.x0 = -(inverse.xx * m.x0 + inverse.xy * m.y0);
    inverse.y0 = -(inverse.yx * m.x0 + inverse.yy * m.y0);
    return true;
}
} // namespace

/* Initialize the portable gradient description without allocating or retaining any pointers. */
extern "C" FVM_Result fvm_gradient_init(FVM_Gradient *gradient)
{
    if (!gradient)
        return FVM_INVALID_ARGUMENT;
    gradient->type = FVM_GRADIENT_LINEAR;
    gradient->matrix.xx = gradient->matrix.yy = 1;
    gradient->matrix.xy = gradient->matrix.yx = gradient->matrix.x0 = gradient->matrix.y0 = 0;
    gradient->stops = 0;
    gradient->count = 0;
    for (unsigned int i = 0; i < 4; ++i)
    {
        gradient->multiply[i] = 1;
        gradient->add[i] = 0;
    }
    return FVM_OK;
}

/* Sample straight RGBA at pixel centers, then apply an antialiased path mask and source-over. */
extern "C" FVM_Result fvm_path_draw_gradient(const FVM_Path *path, FVM_RasterSurface *surface,
                                             const FVM_Gradient *gradient, int even_odd)
{
    return fvm_path_draw_gradient_quality(path, surface, gradient, even_odd, true);
}
/* Apply context-selected edge coverage without changing public standalone defaults. */
FVM_Result fvm_path_draw_gradient_quality(const FVM_Path *path, FVM_RasterSurface *surface,
                                             const FVM_Gradient *gradient, int even_odd, bool antialias)
{
    FVM_Affine inverse;
    if (!validate(path, surface, gradient, even_odd, inverse))
        return FVM_INVALID_ARGUMENT;
#if !defined(FVM_HAS_AGG) && !defined(FVM_HAS_CAIRO)
    (void)antialias;
    return FVM_UNSUPPORTED;
#else
    try
    {
        size_t stride = (size_t)surface->width * 4;
        fvm_detail::vector<unsigned char> pixels(stride * surface->height);
        for (unsigned int y = 0; y < surface->height; ++y)
            for (unsigned int x = 0; x < surface->width; ++x)
            {
                double gx = inverse.xx * (x + 0.5) + inverse.xy * (y + 0.5) + inverse.x0;
                double gy = inverse.yx * (x + 0.5) + inverse.yy * (y + 0.5) + inverse.y0;
                double ratio = gradient->type == FVM_GRADIENT_LINEAR ? (gx + 1) / 2
                                                                     : std::sqrt(gx * gx + gy * gy);
                FVM_RasterColor color = sample(*gradient, ratio);
                unsigned char *p = pixels.begin() + y * stride + x * 4;
                p[0] = color.r;
                p[1] = color.g;
                p[2] = color.b;
                p[3] = color.a;
            }
        FVM_RasterSurface source = {pixels.begin(), surface->width, surface->height, stride};
        return fvm_path_mask_quality(path, surface, &source, even_odd, antialias);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
#endif
}
