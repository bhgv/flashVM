/* FlashVM: clipped BitmapData pixel copying, optional alpha masks and source-over composition. */
#include "bitmap_state.hpp"
#include "../core/native_call_guard.hpp"
#include <cmath>
#include <cstring>

namespace
{
using namespace fvm_geometry_detail;


/* Apply script truthiness to the optional mergeAlpha flag without coercion callbacks. */
bool boolean(const FVM_Value &value)
{
    if (value.type == FVM_VALUE_BOOLEAN)
    {
        return value.boolean != 0;
    }
    if (value.type == FVM_VALUE_NUMBER)
    {
        return value.number != 0 && value.number == value.number;
    }
    if (value.type == FVM_VALUE_STRING)
    {
        return fvm_value_string_size(&value) != 0;
    }
    return value.type == FVM_VALUE_OBJECT;
}


/* Reject nonfinite coordinates and bound conversion before clipping integer pixel regions. */
bool coordinate(double value)
{
    return value >= -2147483648.0 && value <= 2147483647.0;
}


/* Compose straight RGBA source-over with bounded integer arithmetic and nearest rounding. */
void compose(unsigned char *destination, const unsigned char *source)
{
    unsigned long sa = source[3], da = destination[3], inverse = 255 - sa;
    unsigned long alpha = sa * 255 + da * inverse;
    for (unsigned i = 0; i < 3; ++i)
    {
        unsigned long numerator = source[i] * sa * 255 + destination[i] * da * inverse;
        destination[i] = alpha ? (unsigned char)((numerator + alpha / 2) / alpha) : 0;
    }
    destination[3] = (unsigned char)((alpha + 127) / 255);
}
} // namespace


/* [GC:] Copy a clipped source/alpha snapshot before changing the receiver's owned pixel allocation.
 */
FVM_Result
fvm_bitmap_copy_pixels(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out, void *)
{
    fvm_value_undefined(out);


    /* [GC:] Keep receivers and argument edges protected throughout coercion and snapshot
     * allocation. */
    FVM_NativeCallGuard guard(c);
    FVM_Object *source = 0, *rect = 0, *point = 0, *alpha_object = 0;
    const char *rect_names[] = {"x", "y", "width", "height"};
    const char *point_names[] = {"x", "y"};
    double box[4], destination[2], alpha_point[2] = {0, 0};
    FVM_Result result = fvm_geometry_detail::object_argument(c, n, args, 0, source);
    if (result == FVM_OK)
    {
        result = fvm_geometry_detail::object_argument(c, n, args, 1, rect);
    }
    if (result == FVM_OK)
    {
        result = fvm_geometry_detail::numbers(c, rect, rect_names, 4, box);
    }
    if (result == FVM_OK)
    {
        result = fvm_geometry_detail::object_argument(c, n, args, 2, point);
    }
    if (result == FVM_OK)
    {
        result = fvm_geometry_detail::numbers(c, point, point_names, 2, destination);
    }
    if (result == FVM_OK && n > 3 && args[3].type != FVM_VALUE_NULL && args[3].type != FVM_VALUE_UNDEFINED)
    {
        result = fvm_geometry_detail::object_argument(c, n, args, 3, alpha_object);
    }
    if (result == FVM_OK && alpha_object && n > 4 && args[4].type != FVM_VALUE_NULL &&
        args[4].type != FVM_VALUE_UNDEFINED)
    {
        result = fvm_geometry_detail::object_argument(c, n, args, 4, point);
        if (result == FVM_OK)
        {
            result = fvm_geometry_detail::numbers(c, point, point_names, 2, alpha_point);
        }
    }
    if (result != FVM_OK)
    {
        return result;
    }
    /* [GC:] Resolve native buffers after all user getters can have disposed their objects. */
    FVM_BitmapState *target = fvm_bitmap_pixels(self), *input = fvm_bitmap_pixels(source);
    /* [GC:] The optional managed bitmap owns this borrowed mask allocation. */
    FVM_BitmapState *alpha = alpha_object ? fvm_bitmap_pixels(alpha_object) : 0;
    if (!target || !input || target->disposed || input->disposed || (alpha_object && (!alpha || alpha->disposed)))
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        if (!coordinate(box[i]))
        {
            return FVM_OK;
        }
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!coordinate(destination[i]) || !coordinate(alpha_point[i]))
        {
            return FVM_OK;
        }
    }
    double sx = std::floor(box[0]), sy = std::floor(box[1]);
    double dx = std::floor(destination[0]), dy = std::floor(destination[1]);
    double ax = std::floor(alpha_point[0]), ay = std::floor(alpha_point[1]);
    double left = 0, top = 0, right = std::floor(box[2]), bottom = std::floor(box[3]);
    if (left < -sx)
    {
        left = -sx;
    }
    if (left < -dx)
    {
        left = -dx;
    }
    if (top < -sy)
    {
        top = -sy;
    }
    if (top < -dy)
    {
        top = -dy;
    }
    if (right > input->width - sx)
    {
        right = input->width - sx;
    }
    if (right > target->width - dx)
    {
        right = target->width - dx;
    }
    if (bottom > input->height - sy)
    {
        bottom = input->height - sy;
    }
    if (bottom > target->height - dy)
    {
        bottom = target->height - dy;
    }
    if (alpha)
    {
        if (left < -ax)
        {
            left = -ax;
        }
        if (top < -ay)
        {
            top = -ay;
        }
        if (right > alpha->width - ax)
        {
            right = alpha->width - ax;
        }
        if (bottom > alpha->height - ay)
        {
            bottom = alpha->height - ay;
        }
    }
    if (!(left < right && top < bottom))
    {
        return FVM_OK;
    }
    unsigned width = (unsigned)(right - left), height = (unsigned)(bottom - top);
    fvm_detail::vector<unsigned char> snapshot((size_t)width * height * 4);
    /* Read all input and mask pixels first, including when either aliases the destination. */
    for (unsigned y = 0; y < height; ++y)
    {
        for (unsigned x = 0; x < width; ++x)
        {
            unsigned char *pixel = snapshot.begin() + ((size_t)y * width + x) * 4;
            std::memcpy(pixel,
                        input->pixels.begin() + ((size_t)(sy + top + y) * input->width + (size_t)(sx + left + x)) * 4,
                        4);
            if (alpha)
            {
                unsigned mask =
                    alpha->pixels[((size_t)(ay + top + y) * alpha->width + (size_t)(ax + left + x)) * 4 + 3];
                pixel[3] = (unsigned char)((pixel[3] * mask + 127) / 255);
            }
        }
    }
    bool merge = !target->transparent || (n > 5 && boolean(args[5]));
    for (unsigned y = 0; y < height; ++y)
    {
        for (unsigned x = 0; x < width; ++x)
        {
            const unsigned char *pixel = snapshot.begin() + ((size_t)y * width + x) * 4;
            unsigned char *destination_pixel =
                target->pixels.begin() + ((size_t)(dy + top + y) * target->width + (size_t)(dx + left + x)) * 4;
            if (merge)
            {
                compose(destination_pixel, pixel);
            }
            else
            {
                std::memcpy(destination_pixel, pixel, 4);
            }
            if (!target->transparent)
            {
                destination_pixel[3] = 255;
            }
        }
    }
    return FVM_OK;
}
