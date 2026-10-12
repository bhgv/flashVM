/* FlashVM: bounded bitmap fills, masked bounds/tests, channel merging and palette remapping. */
#include "bitmap_ops.hpp"
#include "bitmap_state.hpp"
#include "bitmap_region.hpp"
#include <cstring>
#include "../core/native_call_guard.hpp"
#include "flashvm/standard.h"
#include <cmath>
#include <limits>

namespace
{
using namespace fvm_geometry_detail;
/* Read an argument using ActionScript conversion while rejecting nonfinite pixel/color numbers. */
FVM_Result number(FVM_Context *c, unsigned n, const FVM_Value *args, unsigned index, double &out)
{
    FVM_Value value = argument(n, args, index);
    FVM_Result r = fvm_value_to_number(c, &value, &out);
    if (r != FVM_OK)
        return r;
    return out == out && std::fabs(out) <= std::numeric_limits<double>::max()
               ? FVM_OK
               : FVM_INVALID_ARGUMENT;
}
/* Convert a finite color to its low unsigned 32 bits on both ILP32 and LP64 targets. */
unsigned long color_word(double value)
{
    value = std::fmod(value < 0 ? std::ceil(value) : std::floor(value), 4294967296.0);
    return (unsigned long)(value < 0 ? value + 4294967296.0 : value);
}
/* Read a native straight RGBA pixel as an unsigned ARGB comparison word. */
unsigned long pixel_word(const fvm_detail::vector<unsigned char> &pixels, size_t index)
{
    const unsigned char *p = pixels.begin() + index * 4;
    return ((unsigned long)p[3] << 24) | ((unsigned long)p[0] << 16) | ((unsigned long)p[1] << 8) |
           p[2];
}
/* Write one normalized replacement color into unpublished scanline pixels. */
void write_pixel(fvm_detail::vector<unsigned char> &pixels, size_t index, unsigned long color)
{
    unsigned char *p = pixels.begin() + index * 4;
    p[0] = (unsigned char)(color >> 16);
    p[1] = (unsigned char)(color >> 8);
    p[2] = (unsigned char)color;
    p[3] = (unsigned char)(color >> 24);
}
/* Queue each seed only once, growing the span stack within the remaining scratch budget. */
FVM_Result seed(size_t index, fvm_detail::vector<unsigned char> &queued,
                fvm_detail::vector<size_t> &pending, size_t &reserved, size_t count,
                size_t stack_limit)
{
    unsigned char mask = (unsigned char)(1U << (index & 7));
    if (queued[index >> 3] & mask)
        return FVM_OK;
    if (pending.size() == reserved)
    {
        size_t capacity = reserved ? reserved * 2 : 16;
        if (capacity > count)
            capacity = count;
        size_t available = stack_limit >= reserved ? stack_limit - reserved : 0;
        if (capacity > available)
            capacity = available;
        if (capacity <= pending.size())
            return FVM_OUT_OF_MEMORY;
        pending.reserve(capacity);
        reserved = capacity;
    }
    pending.push_back(index);
    queued[index >> 3] |= mask;
    return FVM_OK;
}
/* Find runs touching a filled span in one adjacent row, queuing one seed per matching run. */
FVM_Result adjacent(const fvm_detail::vector<unsigned char> &pixels, unsigned width, unsigned row,
                    unsigned left, unsigned right, unsigned long original,
                    fvm_detail::vector<unsigned char> &queued, fvm_detail::vector<size_t> &pending,
                    size_t &reserved, size_t count, size_t stack_limit)
{
    size_t base = (size_t)row * width;
    unsigned x = left;
    while (x <= right)
    {
        if (pixel_word(pixels, base + x) != original)
        {
            ++x;
            continue;
        }
        FVM_Result r = seed(base + x, queued, pending, reserved, count, stack_limit);
        if (r != FVM_OK)
            return r;
        do
        {
            ++x;
        } while (x <= right && pixel_word(pixels, base + x) == original);
    }
    return FVM_OK;
}
} // namespace

/* [GC:] Fill a protected receiver through independent pixels so allocation failures cannot leak
 * edits. */
FVM_Result fvm_bitmap_flood_fill(FVM_Context *c, FVM_Object *self, unsigned n,
                                 const FVM_Value *args, FVM_Value *out, void *)
{
    /* [GC:] Retain receiver and coercion callbacks while temporary native storage is built. */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    FVM_BitmapState *target = fvm_bitmap_pixels(self);
    if (!target || target->disposed)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    double x = 0, y = 0, input_color = 0;
    FVM_Result r = number(c, n, args, 0, x);
    if (r == FVM_OK)
        r = number(c, n, args, 1, y);
    if (r == FVM_OK)
        r = number(c, n, args, 2, input_color);
    if (r != FVM_OK)
        return r;
    target = fvm_bitmap_pixels(self);
    if (!target || target->disposed)
        return FVM_INVALID_ARGUMENT;
    x = x < 0 ? std::ceil(x) : std::floor(x);
    y = y < 0 ? std::ceil(y) : std::floor(y);
    if (!(x >= 0 && x < target->width && y >= 0 && y < target->height))
        return FVM_OK;
    unsigned long replacement = color_word(input_color);
    if (!target->transparent)
        replacement |= 0xff000000UL;
    size_t start = (size_t)y * target->width + (unsigned)x;
    unsigned long original = pixel_word(target->pixels, start);
    if (replacement == original)
        return FVM_OK;
    size_t count = (size_t)target->width * target->height;
    size_t marks = (count + 7) / 8;
    size_t scratch = target->pixels.size() + marks;
    unsigned long limit = c->runtime->config.memory_limit_bytes;
    if (limit && scratch > limit)
        return FVM_OUT_OF_MEMORY;
    size_t stack_limit = limit ? ((size_t)limit - scratch) / sizeof(size_t) : count;
    fvm_detail::vector<unsigned char> pixels(target->pixels), queued(marks);
    fvm_detail::vector<size_t> pending;
    size_t reserved = 0;
    r = seed(start, queued, pending, reserved, count, stack_limit);
    while (r == FVM_OK && !pending.empty())
    {
        size_t index = pending.back();
        pending.pop_back();
        if (pixel_word(pixels, index) != original)
            continue;
        unsigned row = (unsigned)(index / target->width);
        unsigned left = (unsigned)(index % target->width), right = left;
        size_t base = (size_t)row * target->width;
        while (left && pixel_word(pixels, base + left - 1) == original)
            --left;
        while (right + 1 < target->width && pixel_word(pixels, base + right + 1) == original)
            ++right;
        for (unsigned column = left; column <= right; ++column)
            write_pixel(pixels, base + column, replacement);
        if (row)
            r = adjacent(pixels, target->width, row - 1, left, right, original, queued, pending,
                         reserved, count, stack_limit);
        if (r == FVM_OK && row + 1 < target->height)
            r = adjacent(pixels, target->width, row + 1, left, right, original, queued, pending,
                         reserved, count, stack_limit);
    }
    if (r != FVM_OK)
        return r;
    /* [GC:] Publish the complete pixel allocation while preserving the object's native identity. */
    target->pixels.swap(pixels);
    return FVM_OK;
}

/* [GC:] Scan a protected native bitmap and allocate an independent Rectangle from captured classes.
 */
FVM_Result fvm_bitmap_color_bounds(FVM_Context *c, FVM_Object *self, unsigned n,
                                   const FVM_Value *args, FVM_Value *out, void *u)
{
    /* [GC:] Retain source and lexical Rectangle factories during coercion and result construction.
     */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    FVM_BitmapState *target = fvm_bitmap_pixels(self);
    if (!target || target->disposed)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    double input_mask = 0, input_color = 0;
    FVM_Result r = number(c, n, args, 0, input_mask);
    if (r == FVM_OK)
        r = number(c, n, args, 1, input_color);
    if (r != FVM_OK)
        return r;
    target = fvm_bitmap_pixels(self);
    if (!target || target->disposed)
        return FVM_INVALID_ARGUMENT;
    unsigned long mask = color_word(input_mask), color = color_word(input_color);
    bool find =
        n < 3 || args[2].type == FVM_VALUE_OBJECT ||
        (args[2].type == FVM_VALUE_BOOLEAN && args[2].boolean) ||
        (args[2].type == FVM_VALUE_NUMBER && args[2].number && args[2].number == args[2].number) ||
        (args[2].type == FVM_VALUE_STRING && fvm_value_string_size(args + 2));
    bool any = false;
    unsigned left = target->width, top = target->height, right = 0, bottom = 0;
    for (unsigned y = 0; y < target->height; ++y)
        for (unsigned x = 0; x < target->width; ++x)
            if (((pixel_word(target->pixels, (size_t)y * target->width + x) & mask) == color) ==
                find)
            {
                any = true;
                if (x < left)
                    left = x;
                if (y < top)
                    top = y;
                if (x + 1 > right)
                    right = x + 1;
                if (y + 1 > bottom)
                    bottom = y + 1;
            }
    const char *const names[] = {"x", "y", "width", "height"};
    FVM_Value values[4];
    fvm_value_number(values, any ? left : 0);
    fvm_value_number(values + 1, any ? top : 0);
    fvm_value_number(values + 2, any ? right - left : 0);
    fvm_value_number(values + 3, any ? bottom - top : 0);
    /* [GC:] Reuse the Rectangle class retained by this bitmap's imported callback capture. */
    FVM_Object *klass = 0;
    r = captured_class(c, u, "Rectangle", klass);
    return r == FVM_OK ? create(c, klass, FVM_GEOMETRY_RECTANGLE, names, 4, values, 0, out) : r;
}

/* [GC:] Evaluate masked unsigned comparisons over a copied region before allocation-free writes. */
FVM_Result fvm_bitmap_threshold(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args,
                                FVM_Value *out, void *)
{
    /* [GC:] Retain receiver, source and argument callbacks throughout region and option coercion.
     */
    FVM_NativeCallGuard guard(c);
    fvm_value_number(out, 0);
    FVM_BitmapRegion region;
    FVM_Result r = fvm_bitmap_region_read(c, n, args, region);
    FVM_Value operation, input_operation = fvm_geometry_detail::argument(n, args, 3);
    if (r == FVM_OK)
        r = fvm_value_to_string(c, &input_operation, &operation);
    if (r != FVM_OK)
        return r;
    const char *const operators[] = {"<", "<=", ">", ">=", "==", "!="};
    unsigned comparison = 6;
    for (unsigned i = 0; i < 6; ++i)
        if (fvm_value_string_size(&operation) == std::strlen(operators[i]) &&
            !std::memcmp(operation.string, operators[i], std::strlen(operators[i])))
            comparison = i;
    if (comparison == 6)
        return FVM_INVALID_ARGUMENT;
    double threshold_number = 0, color_number = 0, mask_number = 4294967295.0;
    r = number(c, n, args, 4, threshold_number);
    if (r == FVM_OK && n > 5)
        r = number(c, n, args, 5, color_number);
    if (r == FVM_OK && n > 6)
        r = number(c, n, args, 6, mask_number);
    if (r != FVM_OK)
        return r;
    FVM_BitmapState *target = 0, *input = 0;
    r = fvm_bitmap_region_clip(self, region, target, input);
    if (r != FVM_OK)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    if (!region.width || !region.height)
        return FVM_OK;
    unsigned long mask = color_word(mask_number), threshold = color_word(threshold_number) & mask;
    unsigned long replacement = color_word(color_number);
    if (!target->transparent)
        replacement |= 0xff000000UL;
    bool copy =
        n > 7 &&
        (args[7].type == FVM_VALUE_OBJECT ||
         (args[7].type == FVM_VALUE_BOOLEAN && args[7].boolean) ||
         (args[7].type == FVM_VALUE_NUMBER && args[7].number && args[7].number == args[7].number) ||
         (args[7].type == FVM_VALUE_STRING && fvm_value_string_size(args + 7)));
    fvm_detail::vector<unsigned char> pixels;
    r = fvm_bitmap_region_snapshot(c, region, input, pixels);
    if (r != FVM_OK)
        return r;
    unsigned passed = 0;
    for (unsigned y = 0; y < region.height; ++y)
        for (unsigned x = 0; x < region.width; ++x)
        {
            size_t index = (size_t)y * region.width + x;
            unsigned long source = pixel_word(pixels, index), tested = source & mask;
            bool match = comparison == 0   ? tested < threshold
                         : comparison == 1 ? tested <= threshold
                         : comparison == 2 ? tested > threshold
                         : comparison == 3 ? tested >= threshold
                         : comparison == 4 ? tested == threshold
                                           : tested != threshold;
            if (!match && !copy)
                continue;
            if (match)
                ++passed;
            unsigned long color = match ? replacement : source;
            if (!target->transparent)
                color |= 0xff000000UL;
            write_pixel(target->pixels, (size_t)(region.dy + y) * target->width + region.dx + x,
                        color);
        }
    fvm_value_number(out, passed);
    return FVM_OK;
}

/* [GC:] Blend copied source channels into destination pixels after all coercion and allocations
 * finish. */
FVM_Result fvm_bitmap_merge(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args,
                            FVM_Value *out, void *)
{
    /* [GC:] Protect receiver/source and any public region or coefficient conversion callbacks. */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    FVM_BitmapRegion region;
    FVM_Result r = fvm_bitmap_region_read(c, n, args, region);
    unsigned multipliers[4];
    bool any = false;
    for (unsigned i = 0; r == FVM_OK && i < 4; ++i)
    {
        double value = 0;
        r = number(c, n, args, i + 3, value);
        if (r != FVM_OK)
            return r;
        multipliers[i] = value <= 0 ? 0 : value >= 256 ? 256 : (unsigned)std::floor(value);
        any = any || multipliers[i] != 0;
    }
    if (r != FVM_OK)
        return r;
    FVM_BitmapState *target = 0, *input = 0;
    r = fvm_bitmap_region_clip(self, region, target, input);
    if (r != FVM_OK)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    if (!any || !region.width || !region.height)
        return FVM_OK;
    fvm_detail::vector<unsigned char> pixels;
    r = fvm_bitmap_region_snapshot(c, region, input, pixels);
    if (r != FVM_OK)
        return r;
    for (unsigned y = 0; y < region.height; ++y)
        for (unsigned x = 0; x < region.width; ++x)
        {
            const unsigned char *source = pixels.begin() + ((size_t)y * region.width + x) * 4;
            unsigned char *dest = target->pixels.begin() +
                                  ((size_t)(region.dy + y) * target->width + region.dx + x) * 4;
            for (unsigned channel = 0; channel < 4; ++channel)
                dest[channel] = (unsigned char)((source[channel] * multipliers[channel] +
                                                 dest[channel] * (256 - multipliers[channel])) /
                                                256);
            if (!target->transparent)
                dest[3] = 255;
        }
    return FVM_OK;
}

/* [GC:] Cache supplied public color tables before snapshotting and remapping source channel values.
 */
FVM_Result fvm_bitmap_palette_map(FVM_Context *c, FVM_Object *self, unsigned n,
                                  const FVM_Value *args, FVM_Value *out, void *)
{
    /* [GC:] Protect source/destination and palette objects while indexed getters may run scripts.
     */
    FVM_NativeCallGuard guard(c);
    fvm_value_undefined(out);
    FVM_BitmapRegion region;
    FVM_Result r = fvm_bitmap_region_read(c, n, args, region);
    if (r != FVM_OK)
        return r;
    bool supplied[4] = {false, false, false, false}, any = false;
    for (unsigned channel = 0; channel < 4; ++channel)
    {
        supplied[channel] = n > channel + 3 && args[channel + 3].type != FVM_VALUE_NULL &&
                            args[channel + 3].type != FVM_VALUE_UNDEFINED;
        any = any || supplied[channel];
    }
    size_t cache_bytes = any ? 1024 * sizeof(unsigned long) : 0;
    unsigned long limit = c->runtime->config.memory_limit_bytes;
    if (limit && cache_bytes > limit)
        return FVM_OUT_OF_MEMORY;
    fvm_detail::vector<unsigned long> table;
    if (any)
        table.resize(1024);
    for (unsigned channel = 0; channel < 4; ++channel)
    {
        if (!supplied[channel])
            continue;
        FVM_Object *array = 0;
        r = fvm_geometry_detail::object_argument(c, n, args, channel + 3, array);
        if (r != FVM_OK)
            return r;
        for (unsigned index = 0; index < 256; ++index)
        {
            char key[4];
            unsigned position = 0;
            if (index >= 100)
                key[position++] = (char)('0' + index / 100);
            if (index >= 10)
                key[position++] = (char)('0' + (index / 10) % 10);
            key[position++] = (char)('0' + index % 10);
            key[position] = 0;
            FVM_Value value;
            double number = 0;
            r = fvm_object_get(c, array, key, &value);
            if (r == FVM_OK)
                r = fvm_value_to_number(c, &value, &number);
            if (r != FVM_OK)
                return r;
            table[channel * 256 + index] =
                number == number && std::fabs(number) <= std::numeric_limits<double>::max()
                    ? color_word(number)
                    : 0;
        }
    }
    FVM_BitmapState *target = 0, *input = 0;
    r = fvm_bitmap_region_clip(self, region, target, input);
    if (r != FVM_OK)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    if (!region.width || !region.height)
        return FVM_OK;
    size_t snapshot_bytes = (size_t)region.width * region.height * 4;
    if (limit && (snapshot_bytes > limit || cache_bytes > limit - snapshot_bytes))
        return FVM_OUT_OF_MEMORY;
    fvm_detail::vector<unsigned char> pixels;
    r = fvm_bitmap_region_snapshot(c, region, input, pixels);
    if (r != FVM_OK)
        return r;
    const unsigned shifts[] = {16, 8, 0, 24};
    for (unsigned y = 0; y < region.height; ++y)
        for (unsigned x = 0; x < region.width; ++x)
        {
            const unsigned char *source = pixels.begin() + ((size_t)y * region.width + x) * 4;
            unsigned long color = 0;
            for (unsigned channel = 0; channel < 4; ++channel)
            {
                unsigned long value = supplied[channel]
                                          ? table[channel * 256 + source[channel]]
                                          : (unsigned long)source[channel] << shifts[channel];
                color = (color + value) & 0xffffffffUL;
            }
            if (!target->transparent)
                color |= 0xff000000UL;
            write_pixel(target->pixels, (size_t)(region.dy + y) * target->width + region.dx + x,
                        color);
        }
    return FVM_OK;
}

namespace
{
/* Convert a public numeric coordinate to signed 32-bit semantics without host integer overflow. */
double hit_integer(double value)
{
    if (!(value == value && std::fabs(value) <= std::numeric_limits<double>::max()))
        return 0;
    unsigned long word = color_word(value);
    return word >= 0x80000000UL ? (double)word - 4294967296.0 : (double)word;
}
/* Read and convert a public point/rectangle tuple; absent fields distinguish invalid geometry. */
FVM_Result hit_tuple(FVM_Context *c, FVM_Object *object, unsigned count, double *values,
                     bool &valid)
{
    const char *const keys[] = {"x", "y", "width", "height"};
    valid = true;
    for (unsigned i = 0; i < count; ++i)
    {
        FVM_Value value;
        FVM_Result r = fvm_object_get(c, object, keys[i], &value);
        if (r != FVM_OK)
            return r;
        if (value.type == FVM_VALUE_UNDEFINED)
            valid = false;
        double number = 0;
        r = fvm_value_to_number(c, &value, &number);
        if (r != FVM_OK)
            return r;
        values[i] = hit_integer(number);
    }
    return FVM_OK;
}
/* Coerce and clamp an alpha threshold, keeping the omitted second threshold separate. */
FVM_Result hit_threshold(FVM_Context *c, unsigned n, const FVM_Value *args, unsigned index,
                         unsigned fallback, unsigned &threshold)
{
    double value = fallback;
    FVM_Result r = FVM_OK;
    if (n > index)
        r = fvm_value_to_number(c, args + index, &value);
    if (r != FVM_OK)
        return r;
    value = hit_integer(value);
    threshold = value < 0 ? 0 : value > 255 ? 255 : (unsigned)value;
    return FVM_OK;
}
} // namespace
/* [GC:] Resolve geometry and thresholds before refreshing native pixel storage for a read-only
 * scan. */
FVM_Result fvm_bitmap_hit_test(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args,
                               FVM_Value *out, void *)
{
    /* [GC:] Prevent collection of receiver, comparison object and getter results during coercion.
     */
    FVM_NativeCallGuard guard(c);
    fvm_value_number(out, -1);
    FVM_BitmapState *initial = fvm_bitmap_pixels(self);
    if (!initial || initial->disposed)
        return FVM_OK;
    FVM_Object *first = 0, *other = 0;
    if (n < 1 || args[0].type != FVM_VALUE_OBJECT || !args[0].object || args[0].object->owner != c)
    {
        fvm_value_number(out, -2);
        return FVM_OK;
    }
    first = args[0].object;
    double first_xy[2], geometry[4] = {0, 0, 1, 1};
    bool valid = false;
    FVM_Result r = hit_tuple(c, first, 2, first_xy, valid);
    if (r != FVM_OK)
        return r;
    if (!valid)
    {
        fvm_value_number(out, -2);
        return FVM_OK;
    }
    unsigned first_alpha = 0, second_alpha = 1;
    r = hit_threshold(c, n, args, 1, 0, first_alpha);
    if (r != FVM_OK)
        return r;
    if (n < 3 || args[2].type != FVM_VALUE_OBJECT || !args[2].object || args[2].object->owner != c)
    {
        fvm_value_number(out, -3);
        return FVM_OK;
    }
    other = args[2].object;
    bool bitmap = fvm_bitmap_pixels(other) != 0, point = false;
    if (bitmap)
    {
        if (fvm_bitmap_pixels(other)->disposed)
        {
            fvm_value_number(out, -3);
            return FVM_OK;
        }
        if (n < 4 || args[3].type != FVM_VALUE_OBJECT || !args[3].object ||
            args[3].object->owner != c)
        {
            fvm_value_number(out, -4);
            return FVM_OK;
        }
        r = hit_tuple(c, args[3].object, 2, geometry, valid);
        if (r != FVM_OK)
            return r;
        if (!valid)
        {
            fvm_value_number(out, -4);
            return FVM_OK;
        }
        r = hit_threshold(c, n, args, 4, 1, second_alpha);
    }
    else
    {
        r = hit_tuple(c, other, 2, geometry, valid);
        if (r != FVM_OK)
            return r;
        if (!valid)
        {
            fvm_value_number(out, -3);
            return FVM_OK;
        }
        FVM_Value width, height;
        r = fvm_object_get(c, other, "width", &width);
        if (r == FVM_OK)
            r = fvm_object_get(c, other, "height", &height);
        if (r != FVM_OK)
            return r;
        point = width.type == FVM_VALUE_UNDEFINED && height.type == FVM_VALUE_UNDEFINED;
        if (!point)
        {
            if (width.type == FVM_VALUE_UNDEFINED || height.type == FVM_VALUE_UNDEFINED)
            {
                fvm_value_number(out, -3);
                return FVM_OK;
            }
            r = fvm_value_to_number(c, &width, geometry + 2);
            if (r == FVM_OK)
                r = fvm_value_to_number(c, &height, geometry + 3);
            geometry[2] = hit_integer(geometry[2]);
            geometry[3] = hit_integer(geometry[3]);
        }
    }
    if (r != FVM_OK)
        return r;
    FVM_BitmapState *target = fvm_bitmap_pixels(self);
    if (!target || target->disposed)
        return FVM_OK;
    FVM_BitmapState *comparison = bitmap ? fvm_bitmap_pixels(other) : 0;
    if (bitmap && (!comparison || comparison->disposed))
    {
        fvm_value_number(out, -3);
        return FVM_OK;
    }
    if (bitmap)
    {
        geometry[2] = comparison->width;
        geometry[3] = comparison->height;
    }
    double offset_x = geometry[0] - first_xy[0], offset_y = geometry[1] - first_xy[1];
    double left = offset_x > 0 ? offset_x : 0, top = offset_y > 0 ? offset_y : 0;
    double right = offset_x + geometry[2], bottom = offset_y + geometry[3];
    if (right > target->width)
        right = target->width;
    if (bottom > target->height)
        bottom = target->height;
    bool hit = false;
    if (left < right && top < bottom && geometry[2] > 0 && geometry[3] > 0)
    {
        unsigned threshold = point && first_alpha == 0 ? 1 : first_alpha;
        for (unsigned y = (unsigned)top; y < (unsigned)bottom && !hit; ++y)
            for (unsigned x = (unsigned)left; x < (unsigned)right; ++x)
            {
                if (target->pixels[((size_t)y * target->width + x) * 4 + 3] < threshold)
                    continue;
                if (bitmap)
                {
                    unsigned bx = (unsigned)(x - offset_x), by = (unsigned)(y - offset_y);
                    if (comparison->pixels[((size_t)by * comparison->width + bx) * 4 + 3] <
                        second_alpha)
                        continue;
                }
                hit = true;
                break;
            }
    }
    fvm_value_boolean(out, hit ? 1 : 0);
    return FVM_OK;
}
