/* FlashVM: backend-independent gradient sampling composed through the existing vector mask API. */
#include "../render/quality_internal.hpp"
#include "../render/gradient_constants.h"

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

/* Normalize finite phases without integer casts, including negative and very large coordinates. */
double spread_ratio(double ratio, unsigned int spread)
{
    if (spread == FVM_GRADIENT_SPREAD_REPEAT)
        return ratio - std::floor(ratio);
    if (spread == FVM_GRADIENT_SPREAD_REFLECT)
    {
        double phase = std::fmod(ratio, FVM_GRADIENT_REFLECT_PERIOD);
        if (phase < 0) phase += FVM_GRADIENT_REFLECT_PERIOD;
        return phase > 1 ? FVM_GRADIENT_REFLECT_PERIOD - phase : phase;
    }
    /* Preserve legacy pad endpoint/duplicate-stop behavior exactly, without clamping first. */
    return ratio;
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
/* Shared validated paint implementation; width zero retains legacy fill coverage. */
static FVM_Result gradient_paint(const FVM_Path *, FVM_RasterSurface *, const FVM_Gradient *,
                                 double width, int even_odd, unsigned int spread, bool antialias);
/* Apply context-selected edge coverage without changing public standalone defaults. */
FVM_Result fvm_path_draw_gradient_quality(const FVM_Path *path, FVM_RasterSurface *surface,
                                             const FVM_Gradient *gradient, int even_odd, bool antialias)
{
    return fvm_path_draw_gradient_spread_quality(path, surface, gradient, even_odd,
                                                  FVM_GRADIENT_SPREAD_PAD, antialias);
}
/* Borrow all inputs and preserve the public standalone antialias default. */
extern "C" FVM_Result fvm_path_draw_gradient_spread(const FVM_Path *path, FVM_RasterSurface *surface,
                                                    const FVM_Gradient *gradient, int even_odd,
                                                    unsigned int spread)
{
    return fvm_path_draw_gradient_spread_quality(path, surface, gradient, even_odd, spread, true);
}
/* Validate mode/geometry before allocating a temporary source or touching destination pixels. */
FVM_Result fvm_path_draw_gradient_spread_quality(const FVM_Path *path, FVM_RasterSurface *surface,
                                                  const FVM_Gradient *gradient, int even_odd,
                                                  unsigned int spread, bool antialias)
{
    return gradient_paint(path, surface, gradient, 0, even_odd, spread, antialias);
}
/* Public stroke entry keeps antialiasing local and never retains borrowed paint data. */
extern "C" FVM_Result fvm_path_stroke_gradient(const FVM_Path *path, FVM_RasterSurface *surface,
                                               const FVM_Gradient *gradient, double width,
                                               unsigned int spread)
{
    return fvm_path_stroke_gradient_quality(path, surface, gradient, width, spread, true);
}
/* Validate positive stroke width before allocating a sampled source or backend mask. */
FVM_Result fvm_path_stroke_gradient_quality(const FVM_Path *path, FVM_RasterSurface *surface,
                                            const FVM_Gradient *gradient, double width,
                                            unsigned int spread, bool antialias)
{
    if (!(width > 0 && width <= 4096))
        return FVM_INVALID_ARGUMENT;
    return gradient_paint(path, surface, gradient, width, 0, spread, antialias);
}
/* Share the legacy sampler and compositing order for both fill and stroke masks. */
static FVM_Result gradient_paint(const FVM_Path *path, FVM_RasterSurface *surface,
                                 const FVM_Gradient *gradient, double width, int even_odd,
                                 unsigned int spread, bool antialias)
{
    if (spread != FVM_GRADIENT_SPREAD_PAD && spread != FVM_GRADIENT_SPREAD_REFLECT &&
        spread != FVM_GRADIENT_SPREAD_REPEAT)
        return FVM_INVALID_ARGUMENT;
    FVM_Affine inverse;
    if (!validate(path, surface, gradient, even_odd, inverse))
        return FVM_INVALID_ARGUMENT;
#if !defined(FVM_HAS_AGG) && !defined(FVM_HAS_CAIRO)
    (void)antialias;
    (void)width;
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
                FVM_RasterColor color = sample(*gradient, spread_ratio(ratio, spread));
                unsigned char *p = pixels.begin() + y * stride + x * 4;
                p[0] = color.r;
                p[1] = color.g;
                p[2] = color.b;
                p[3] = color.a;
            }
        FVM_RasterSurface source = {pixels.begin(), surface->width, surface->height, stride};
        return fvm_path_mask_width_quality(path, surface, &source, width, even_odd, antialias);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
#endif
}
