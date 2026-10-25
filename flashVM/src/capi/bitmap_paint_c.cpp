/* FlashVM: affine bitmap sampling, repeat/edge extension and alpha-aware bilinear filtering. */
#include "../render/quality_internal.hpp"

#include "flashvm/bitmap_paint.h"
#include "../details/vector.hpp"
#include <cmath>
#include <climits>
#include <new>

namespace
{


/* Validate a bounded host-owned RGBA surface before reading or allocating image storage. */
bool surface(const FVM_RasterSurface *value)
{
    return value && value->pixels && value->width && value->height && value->width <= 4096 && value->height <= 4096 &&
           value->stride >= (size_t)value->width * 4 && value->stride <= (size_t)INT_MAX / value->height;
}


/* Check a finite scalar without relying on C99 isfinite or host floating-point exception state. */
bool bounded(double value, double limit)
{
    return value >= -limit && value <= limit;
}


/* Validate immutable inputs and compute the inverse source-to-destination matrix. */
bool validate(const FVM_Path *path,
              FVM_RasterSurface *destination,
              const FVM_BitmapPaint *paint,
              int even_odd,
              FVM_Affine &inverse)
{
    if (!path || !surface(destination) || !paint || !surface(paint->source) ||
        (paint->repeat != 0 && paint->repeat != 1) || (paint->smoothing != 0 && paint->smoothing != 1) ||
        (even_odd != 0 && even_odd != 1))
    {
        return false;
    }
    for (size_t i = 0; i < fvm_path_count(path); ++i)
    {
        FVM_PathCommand command;
        if (fvm_path_get(path, i, &command) != FVM_OK)
        {
            return false;
        }
        unsigned count = command.verb == FVM_PATH_CLOSE       ? 0
                         : command.verb == FVM_PATH_CUBIC     ? 3
                         : command.verb == FVM_PATH_QUADRATIC ? 2
                                                              : 1;
        for (unsigned j = 0; j < count; ++j)
        {
            if (!bounded(command.x[j], 4096) || !bounded(command.y[j], 4096))
            {
                return false;
            }
        }
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        if (!bounded(paint->multiply[i], 65536) || !bounded(paint->add[i], 65536))
        {
            return false;
        }
    }
    const FVM_Affine &m = paint->matrix;
    if (!bounded(m.xx, 1000000) || !bounded(m.yx, 1000000) || !bounded(m.xy, 1000000) || !bounded(m.yy, 1000000) ||
        !bounded(m.x0, 1000000) || !bounded(m.y0, 1000000))
    {
        return false;
    }
    double determinant = m.xx * m.yy - m.xy * m.yx;
    if (!(std::fabs(determinant) >= 1e-20))
    {
        return false;
    }
    inverse.xx = m.yy / determinant;
    inverse.xy = -m.xy / determinant;
    inverse.yx = -m.yx / determinant;
    inverse.yy = m.xx / determinant;
    inverse.x0 = -(inverse.xx * m.x0 + inverse.xy * m.y0);
    inverse.y0 = -(inverse.yx * m.x0 + inverse.yy * m.y0);
    return true;
}
#if defined(FVM_HAS_AGG) || defined(FVM_HAS_CAIRO)


/* Select a finite integer pixel coordinate with wrapping or edge extension before narrowing. */
unsigned index(double value, unsigned count, bool repeat)
{
    if (repeat)
    {
        value = std::fmod(value, (double)count);
        if (value < 0)
        {
            value += count;
        }
    }
    else
    {
        if (value < 0)
        {
            return 0;
        }
        if (value >= count)
        {
            return count - 1;
        }
    }
    return (unsigned)value;
}


/* Clamp and round a final straight RGBA color component. */
unsigned char component(double value)
{
    return !(value > 0) ? 0 : value >= 255 ? 255 : (unsigned char)(value + 0.5);
}


/* Sample pixel centers with alpha-aware filtering, then apply the composed color transform. */
void sample(const FVM_BitmapPaint &paint, double x, double y, unsigned char *out)
{
    const FVM_RasterSurface &source = *paint.source;
    double values[4] = {0, 0, 0, 0};
    if (!paint.smoothing)
    {
        const unsigned char *pixel = source.pixels +
                                     index(std::floor(y), source.height, paint.repeat != 0) * source.stride +
                                     index(std::floor(x), source.width, paint.repeat != 0) * 4;
        for (unsigned i = 0; i < 4; ++i)
        {
            values[i] = pixel[i];
        }
    }
    else
    {
        x -= 0.5;
        y -= 0.5;
        double left = std::floor(x), top = std::floor(y), fx = x - left, fy = y - top;
        for (unsigned row = 0; row < 2; ++row)
        {
            for (unsigned column = 0; column < 2; ++column)
            {
                const unsigned char *pixel = source.pixels +
                                             index(top + row, source.height, paint.repeat != 0) * source.stride +
                                             index(left + column, source.width, paint.repeat != 0) * 4;
                double weight = (column ? fx : 1 - fx) * (row ? fy : 1 - fy);
                values[3] += pixel[3] * weight;
                for (unsigned i = 0; i < 3; ++i)
                {
                    values[i] += (double)pixel[i] * pixel[3] * weight;
                }
            }
        }
        for (unsigned i = 0; i < 3; ++i)
        {
            values[i] = values[3] ? values[i] / values[3] : 0;
        }
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        out[i] = component(values[i] * paint.multiply[i] + paint.add[i]);
    }
}
#endif
} // namespace


/* Initialize the complete sampling description without retaining any source pointer. */
extern "C" FVM_Result fvm_bitmap_paint_init(FVM_BitmapPaint *paint)
{
    if (!paint)
    {
        return FVM_INVALID_ARGUMENT;
    }
    paint->source = 0;
    paint->repeat = 1;
    paint->smoothing = 0;
    paint->matrix.xx = paint->matrix.yy = 1;
    paint->matrix.xy = paint->matrix.yx = paint->matrix.x0 = paint->matrix.y0 = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        paint->multiply[i] = 1;
        paint->add[i] = 0;
    }
    return FVM_OK;
}


/* Sample into independent temporary storage before compositing through the backend's path mask. */
extern "C" FVM_Result
fvm_path_draw_bitmap(const FVM_Path *path, FVM_RasterSurface *destination, const FVM_BitmapPaint *paint, int even_odd)
{
    return fvm_path_draw_bitmap_quality(path, destination, paint, even_odd, true);
}


/* Apply context-selected edge coverage without changing public standalone defaults. */
FVM_Result fvm_path_draw_bitmap_quality(
    const FVM_Path *path, FVM_RasterSurface *destination, const FVM_BitmapPaint *paint, int even_odd, bool antialias)
{
    return fvm_path_bitmap_paint_quality(path, destination, paint, 0, even_odd, antialias);
}


/* Sample the complete source before preparing either a fill or round-stroke coverage mask. */
FVM_Result fvm_path_bitmap_paint_quality(const FVM_Path *path,
                                         FVM_RasterSurface *destination,
                                         const FVM_BitmapPaint *paint,
                                         double width,
                                         int even_odd,
                                         bool antialias,
                                         unsigned cap,
                                         unsigned join)
{
    FVM_Affine inverse;
    if (cap > FVM_STROKE_CAP_SQUARE || join > FVM_STROKE_JOIN_BEVEL ||
        !(width >= 0 && width <= FVM_BITMAP_STROKE_MAX_WIDTH) || !validate(path, destination, paint, even_odd, inverse))
    {
        return FVM_INVALID_ARGUMENT;
    }
#if !defined(FVM_HAS_AGG) && !defined(FVM_HAS_CAIRO)
    (void)antialias;
    return FVM_UNSUPPORTED;
#else
    try
    {
        size_t stride = (size_t)destination->width * 4;
        fvm_detail::vector<unsigned char> pixels(stride * destination->height);
        for (unsigned y = 0; y < destination->height; ++y)
        {
            for (unsigned x = 0; x < destination->width; ++x)
            {
                double sx = inverse.xx * (x + 0.5) + inverse.xy * (y + 0.5) + inverse.x0;
                double sy = inverse.yx * (x + 0.5) + inverse.yy * (y + 0.5) + inverse.y0;
                sample(*paint, sx, sy, pixels.begin() + y * stride + x * 4);
            }
        }
        FVM_RasterSurface source = {pixels.begin(), destination->width, destination->height, stride};
        return fvm_path_mask_width_quality(path, destination, &source, width, even_odd, antialias, cap, join);
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
#endif
}
