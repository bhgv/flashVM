/* FlashVM: transactional four-connected scanline flood filling with a bounded pending-span stack.
 */
#include "bitmap_ops.hpp"
#include "bitmap_state.hpp"
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
