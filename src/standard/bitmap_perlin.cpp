/* FlashVM: portable gradient turbulence, octave offsets and seamless bitmap noise staging. */
#include "bitmap_perlin.hpp"
#include "bitmap_state.hpp"
#include "../core/native_call_guard.hpp"
#include "flashvm/standard.h"
#include <cmath>
#include <limits>

namespace
{
/* Detached two-dimensional gradient or octave offset, containing no managed references. */
struct Pair
{
    double x; /* Horizontal gradient component or copied offset. */
    double y; /* Vertical gradient component or copied offset. */
};
/* Convert an unsigned modular seed without overflowing signed 32-bit integers. */
unsigned long word(double value)
{
    if (!(value == value && std::fabs(value) <= std::numeric_limits<double>::max()))
        return 0;
    value = std::fmod(value < 0 ? std::ceil(value) : std::floor(value), 4294967296.0);
    return (unsigned long)(value < 0 ? value + 4294967296.0 : value);
}
/* Read a numeric argument with an explicit default and preserve conversion exceptions. */
FVM_Result numeric(FVM_Context *c, unsigned n, const FVM_Value *a, unsigned index, double fallback,
                   double &out)
{
    out = fallback;
    return n > index ? fvm_value_to_number(c, a + index, &out) : FVM_OK;
}
/* Apply AS2 truth to optional mode flags without numeric user callbacks. */
bool truth(unsigned n, const FVM_Value *a, unsigned index)
{
    if (n <= index)
        return false;
    const FVM_Value &v = a[index];
    return v.type == FVM_VALUE_OBJECT || (v.type == FVM_VALUE_BOOLEAN && v.boolean) ||
           (v.type == FVM_VALUE_NUMBER && v.number && v.number == v.number) ||
           (v.type == FVM_VALUE_STRING && fvm_value_string_size(&v));
}
/* Advance a normalized nonzero Park-Miller seed with signed-32-safe Schrage products. */
unsigned long advance(unsigned long &state)
{
    long next = 16807L * (long)(state % 127773UL) - 2836L * (long)(state / 127773UL);
    if (next <= 0)
        next += 2147483647L;
    state = (unsigned long)next;
    return state;
}
/* Reduce an integral floating lattice coordinate to the bounded permutation table. */
unsigned lattice(double value)
{
    value = std::fmod(value, 256.0);
    return (unsigned)(value < 0 ? value + 256 : value);
}
/* Evaluate cubic interpolation without changing the original gradient dot products. */
double interpolate(double fraction, double a, double b)
{
    double curve = fraction * fraction * (3 - 2 * fraction);
    return a + curve * (b - a);
}
/* Build independent channel gradients and one shared permutation using detached heap buffers. */
class Turbulence
{
    fvm_detail::vector<unsigned> permutation_; /* Bounded shuffled lattice selectors. */
    fvm_detail::vector<Pair> gradients_;       /* Four independently seeded gradient planes. */
    /* Prevent copying heap tables unnecessarily during pixel evaluation. */
    Turbulence(const Turbulence &);
    /* Prevent assigning a partially populated turbulence table. */
    Turbulence &operator=(const Turbulence &);

  public:
    /* Normalize the signed seed, initialize unit gradients and shuffle the shared lattice. */
    explicit Turbulence(double seed) : permutation_(256), gradients_(1024)
    {
        double normalized = seed <= 0 ? std::fmod(-seed, 2147483646.0) + 1 : seed;
        if (normalized > 2147483646.0)
            normalized = 2147483646.0;
        unsigned long state = (unsigned long)normalized;
        for (unsigned channel = 0; channel < 4; ++channel)
            for (unsigned i = 0; i < 256; ++i)
            {
                permutation_[i] = i;
                Pair &g = gradients_[channel * 256 + i];
                g.x = ((double)(advance(state) % 512) - 256) / 256;
                g.y = ((double)(advance(state) % 512) - 256) / 256;
                double length = std::sqrt(g.x * g.x + g.y * g.y);
                if (!length)
                {
                    g.x = 1;
                    g.y = 0;
                }
                else
                {
                    g.x /= length;
                    g.y /= length;
                }
            }
        for (unsigned i = 255; i; --i)
        {
            unsigned j = (unsigned)(advance(state) % 256), saved = permutation_[i];
            permutation_[i] = permutation_[j];
            permutation_[j] = saved;
        }
    }
    /* Sample four neighboring gradients with optional octave-specific lattice wrapping. */
    double sample(unsigned channel, double x, double y, bool stitch, double period_x,
                  double period_y) const
    {
        double shifted_x = x + 4096, shifted_y = y + 4096;
        double whole_x = shifted_x < 0 ? std::ceil(shifted_x) : std::floor(shifted_x);
        double whole_y = shifted_y < 0 ? std::ceil(shifted_y) : std::floor(shifted_y);
        double fx = shifted_x - whole_x, fy = shifted_y - whole_y;
        double xs[] = {whole_x, whole_x + 1}, ys[] = {whole_y, whole_y + 1};
        if (stitch)
            for (unsigned i = 0; i < 2; ++i)
            {
                if (xs[i] >= 4096 + period_x)
                    xs[i] -= period_x;
                if (ys[i] >= 4096 + period_y)
                    ys[i] -= period_y;
            }
        double dots[2][2];
        for (unsigned row = 0; row < 2; ++row)
            for (unsigned column = 0; column < 2; ++column)
            {
                unsigned hash =
                    permutation_[(permutation_[lattice(xs[column])] + lattice(ys[row])) & 255];
                const Pair &g = gradients_[channel * 256 + hash];
                dots[row][column] = (fx - column) * g.x + (fy - row) * g.y;
            }
        return interpolate(fy, interpolate(fx, dots[0][0], dots[0][1]),
                           interpolate(fx, dots[1][0], dots[1][1]));
    }
    /* Accumulate independent octave offsets with doubling frequency and halving amplitude. */
    double value(unsigned channel, unsigned x, unsigned y, double frequency_x, double frequency_y,
                 unsigned octaves, const Pair *offsets, bool fractal, bool stitch, double period_x,
                 double period_y) const
    {
        double sum = 0, scale = 1;
        for (unsigned octave = 0; octave < octaves; ++octave)
        {
            double noise = sample(channel, (x + offsets[octave].x) * frequency_x * scale,
                                  (y + offsets[octave].y) * frequency_y * scale, stitch,
                                  period_x * scale, period_y * scale);
            sum += (fractal ? noise : std::fabs(noise)) / scale;
            scale *= 2;
        }
        return sum;
    }
};
/* Select the closest tile-compatible frequency, handling zero lower candidates explicitly. */
double stitched(double frequency, unsigned size)
{
    if (!frequency)
        return 0;
    double lower = std::floor(size * frequency) / size, upper = std::ceil(size * frequency) / size;
    return lower && frequency / lower < upper / frequency ? lower : upper;
}
/* Convert turbulence or fractal sums using Flash's pre-division rounding placement. */
unsigned char byte(double value, bool fractal)
{
    value = fractal ? (value * 255 + 255 + 0.5) / 2 : value * 255 + 0.5;
    return value <= 0 ? 0 : value >= 255 ? 255 : (unsigned char)value;
}
} // namespace
/* [GC:] Stage a complete procedural bitmap after public argument and offset conversions finish. */
FVM_Result fvm_bitmap_perlin_noise(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *a,
                                   FVM_Value *out, void *)
{
    /* [GC:] Protect receiver and public offset values through all reentrant getters/conversions. */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    FVM_BitmapState *initial = fvm_bitmap_pixels(self);
    if (!initial || initial->disposed)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    double base_x = 0, base_y = 0, count = 0, seed_number = 0, channels_number = 7;
    FVM_Result r = numeric(c, n, a, 0, 0, base_x);
    if (r == FVM_OK)
        r = numeric(c, n, a, 1, 0, base_y);
    if (r == FVM_OK)
        r = numeric(c, n, a, 2, 0, count);
    if (r == FVM_OK)
        r = numeric(c, n, a, 3, 0, seed_number);
    if (r == FVM_OK)
        r = numeric(c, n, a, 6, 7, channels_number);
    if (r != FVM_OK)
        return r;
    if (!(std::fabs(base_x) <= 1000000 && std::fabs(base_y) <= 1000000 && count >= 0 &&
          count <= 16))
        return FVM_INVALID_ARGUMENT;
    if ((base_x && std::fabs(base_x) < 0.000001) || (base_y && std::fabs(base_y) < 0.000001))
        return FVM_INVALID_ARGUMENT;
    unsigned octaves = (unsigned)std::floor(count),
             channels = (unsigned)(word(channels_number) & 15);
    bool stitch = truth(n, a, 4), fractal = truth(n, a, 5), gray = truth(n, a, 7);
    Pair offsets[16];
    for (unsigned i = 0; i < 16; ++i)
        offsets[i].x = offsets[i].y = 0;
    if (n > 8 && a[8].type != FVM_VALUE_NULL && a[8].type != FVM_VALUE_UNDEFINED)
    {
        FVM_Object *array = 0;
        r = fvm_geometry_detail::object_argument(c, n, a, 8, array);
        if (r != FVM_OK)
            return r;
        for (unsigned i = 0; i < octaves; ++i)
        {
            char key[3];
            unsigned position = 0;
            if (i >= 10)
                key[position++] = '1';
            key[position++] = (char)('0' + i % 10);
            key[position] = 0;
            FVM_Value entry;
            r = fvm_object_get(c, array, key, &entry);
            if (r != FVM_OK)
                return r;
            if (entry.type == FVM_VALUE_UNDEFINED || entry.type == FVM_VALUE_NULL)
                continue;
            if (entry.type != FVM_VALUE_OBJECT || !entry.object || entry.object->owner != c)
                return FVM_INVALID_ARGUMENT;
            const char *const names[] = {"x", "y"};
            double xy[2];
            r = fvm_geometry_detail::numbers(c, entry.object, names, 2, xy);
            if (r != FVM_OK)
                return r;
            if (!(std::fabs(xy[0]) <= 1000000 && std::fabs(xy[1]) <= 1000000))
                return FVM_INVALID_ARGUMENT;
            offsets[i].x = xy[0];
            offsets[i].y = xy[1];
        }
    }
    FVM_BitmapState *target = fvm_bitmap_pixels(self);
    if (!target || target->disposed)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    size_t table_bytes = 256 * sizeof(unsigned) + 1024 * sizeof(Pair);
    unsigned long limit = c->runtime->config.memory_limit_bytes;
    if (limit && (target->pixels.size() > limit || table_bytes > limit - target->pixels.size()))
        return FVM_OUT_OF_MEMORY;
    unsigned long seed = word(seed_number);
    double signed_seed = seed >= 0x80000000UL ? (double)seed - 4294967296.0 : (double)seed;
    Turbulence generator(signed_seed);
    double frequency_x = base_x ? 1 / base_x : 0, frequency_y = base_y ? 1 / base_y : 0;
    if (stitch)
    {
        frequency_x = stitched(frequency_x, target->width);
        frequency_y = stitched(frequency_y, target->height);
    }
    double period_x = std::floor(target->width * frequency_x + 0.5),
           period_y = std::floor(target->height * frequency_y + 0.5);
    /* [GC:] Keep replacement pixels local until the owning bitmap can publish a complete image. */
    fvm_detail::vector<unsigned char> pixels(target->pixels.size());
    for (unsigned y = 0; y < target->height; ++y)
        for (unsigned x = 0; x < target->width; ++x)
        {
            unsigned char *pixel = pixels.begin() + ((size_t)y * target->width + x) * 4;
            unsigned selected = 0;
            for (unsigned channel = 0; channel < 4; ++channel)
            {
                bool enabled = channel < 3 && gray ? true : (channels & (1U << channel)) != 0;
                unsigned gradient = gray ? (channel == 3 ? 1 : 0) : selected;
                double value = channel == 3 ? 1 : -1;
                if (enabled)
                {
                    value = generator.value(gradient, x, y, frequency_x, frequency_y, octaves,
                                            offsets, fractal, stitch, period_x, period_y);
                    ++selected;
                }
                pixel[channel] = byte(value, fractal);
            }
            if (!target->transparent)
                pixel[3] = 255;
            if (!pixel[3])
                pixel[0] = pixel[1] = pixel[2] = 0;
        }
    /* [GC:] Preserve bitmap identity and publish the completed procedural pixel buffer atomically.
     */
    target->pixels.swap(pixels);
    return FVM_OK;
}
