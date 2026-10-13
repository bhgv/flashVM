/* FlashVM: bounded pixel dissolve using a deterministic permutation and staged region buffers.
 * The permutation is a FlashVM model; exact historical Flash pixel order is not certified. */
#include "bitmap_dissolve.hpp"
#include "bitmap_region.hpp"
#include "flashvm/standard.h"
#include <cmath>
#include <cstring>
#include <limits>

namespace
{
/* Read an optional public number without retaining native pixel storage across user conversion. */
FVM_Result argument_number(FVM_Context *c, unsigned n, const FVM_Value *args, unsigned index,
                           double fallback, double &value)
{
    value = fallback;
    return index < n ? fvm_value_to_number(c, args + index, &value) : FVM_OK;
}
/* Reduce floating script integers to unsigned 32 bits without signed overflow or invalid casts. */
unsigned long word(double value)
{
    if (!(value == value) || std::fabs(value) > std::numeric_limits<double>::max())
        return 0;
    value = std::fmod(value < 0 ? std::ceil(value) : std::floor(value), 4294967296.0);
    return (unsigned long)(value < 0 ? value + 4294967296.0 : value);
}
/* Permute a bounded even-width counter by exchanging halves and mixing the low half into high. */
unsigned long permute(unsigned long cursor, unsigned half)
{
    unsigned long mask = (1UL << half) - 1;
    unsigned long low = cursor & mask;
    unsigned long high = cursor >> half;
    return ((high ^ ((low * low + 1) & mask)) << half) | low;
}
/* Store one normalized color in unpublished RGBA memory. */
void fill(unsigned char *pixel, unsigned long color, bool transparent)
{
    unsigned alpha = transparent ? (unsigned)(color >> 24) : 255;
    pixel[0] = alpha ? (unsigned char)(color >> 16) : 0;
    pixel[1] = alpha ? (unsigned char)(color >> 8) : 0;
    pixel[2] = alpha ? (unsigned char)color : 0;
    pixel[3] = (unsigned char)alpha;
}
} // namespace

/* [GC:] Revalidate receiver/source after public coercions and stage all pixel edits before commit.
 */
FVM_Result fvm_bitmap_pixel_dissolve(FVM_Context *c, FVM_Object *self, unsigned n,
                                     const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_number(out, -1);
    FVM_BitmapState *target = fvm_bitmap_pixels(self);
    if (!target || target->disposed)
        return FVM_OK;

    FVM_BitmapRegion region;
    FVM_Result result = fvm_bitmap_region_read(c, n, args, region);
    double seed = 0, count = 0, color = 0;
    if (result == FVM_OK)
        result = argument_number(c, n, args, 3, 0, seed);
    if (result == FVM_OK && n > 4)
        result = argument_number(c, n, args, 4, 0, count);
    if (result == FVM_OK)
        result = argument_number(c, n, args, 5, 0, color);
    if (result != FVM_OK)
        return result;
    if (!(count >= 0 && count <= std::numeric_limits<double>::max()))
        return FVM_INVALID_ARGUMENT;

    /* Same-object dissolve fills the source area; public destPoint is coerced but not used. */
    bool same = region.source == self;
    if (same)
    {
        region.destination[0] = region.box[0];
        region.destination[1] = region.box[1];
    }
    FVM_BitmapState *source = 0;
    result = fvm_bitmap_region_clip(self, region, target, source);
    if (result != FVM_OK)
        return FVM_OK;
    unsigned long area = (unsigned long)region.width * region.height;
    if (!area)
    {
        fvm_value_number(out, 0);
        return FVM_OK;
    }
    if (n <= 4)
        count = std::floor((double)area / 30);
    unsigned long steps = count >= area ? area : (unsigned long)std::floor(count);
    unsigned bits = 2;
    while ((1UL << bits) < area)
        bits += 2;
    unsigned long domain = 1UL << bits;
    unsigned long cursor = word(seed) & (domain - 1);
    if (!steps)
    {
        fvm_value_number(out, (double)cursor);
        return FVM_OK;
    }

    /* Reserve both unpublished output and copied input before performing any destination write. */
    size_t bytes = (size_t)area * 4;
    unsigned long limit = c->runtime->config.memory_limit_bytes;
    if (limit && bytes > limit / (same ? 1 : 2))
        return FVM_OUT_OF_MEMORY;
    fvm_detail::vector<unsigned char> output(bytes), input;
    if (!same)
    {
        result = fvm_bitmap_region_snapshot(c, region, source, input);
        if (result != FVM_OK)
            return result;
    }
    for (unsigned y = 0; y < region.height; ++y)
        std::memcpy(output.begin() + (size_t)y * region.width * 4,
                    target->pixels.begin() +
                        ((size_t)(region.dy + y) * target->width + region.dx) * 4,
                    (size_t)region.width * 4);
    unsigned long replacement = word(color);
    for (unsigned long i = 0; i < steps; ++i)
    {
        unsigned long index;
        do
        {
            cursor = (cursor + 1) & (domain - 1);
            index = permute(cursor, bits / 2);
        } while (index >= area);
        unsigned char *pixel = output.begin() + (size_t)index * 4;
        if (same)
            fill(pixel, replacement, target->transparent);
        else
        {
            std::memcpy(pixel, input.begin() + (size_t)index * 4, 4);
            if (!target->transparent)
                pixel[3] = 255;
        }
    }

    /* [GC:] No allocation, callback or collector safe point occurs during final pixel publication.
     */
    for (unsigned y = 0; y < region.height; ++y)
        std::memcpy(target->pixels.begin() +
                        ((size_t)(region.dy + y) * target->width + region.dx) * 4,
                    output.begin() + (size_t)y * region.width * 4, (size_t)region.width * 4);
    fvm_value_number(out, (double)cursor);
    return FVM_OK;
}
