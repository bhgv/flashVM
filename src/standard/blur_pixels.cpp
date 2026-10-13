/* FlashVM: separable fractional box blur with padded premultiplied staging and bounded output. */
#include "blur.hpp"
#include <cmath>
#include <cstring>
namespace
{
/* Read one padded plane component, using transparent black outside the working canvas. */
double sample(const fvm_detail::vector<double> &plane, size_t base, size_t stride, int coordinate,
              unsigned length, unsigned channel)
{
    return coordinate < 0 || (unsigned)coordinate >= length
               ? 0
               : plane[base + (size_t)coordinate * stride + channel];
}
/* Evaluate a fractional one-dimensional centered box in linear time with a sliding interior sum. */
void pass(const fvm_detail::vector<double> &input, fvm_detail::vector<double> &output,
          unsigned width, unsigned height, double amount, bool horizontal)
{
    unsigned radius = fvm_blur_radius(amount);
    if (!radius)
    {
        for (size_t i = 0; i < input.size(); ++i)
            output[i] = input[i];
        return;
    }
    unsigned length = horizontal ? width : height;
    unsigned lines = horizontal ? height : width;
    size_t stride = horizontal ? 4 : (size_t)width * 4;
    double edge = (amount - (2 * radius - 1)) / 2;
    for (unsigned line = 0; line < lines; ++line)
    {
        size_t base = horizontal ? (size_t)line * width * 4 : (size_t)line * 4;
        double sums[4] = {0, 0, 0, 0};
        for (int x = 1 - (int)radius; x < (int)radius; ++x)
            for (unsigned channel = 0; channel < 4; ++channel)
                sums[channel] += sample(input, base, stride, x, length, channel);
        for (unsigned position = 0; position < length; ++position)
            for (unsigned channel = 0; channel < 4; ++channel)
            {
                double left =
                    sample(input, base, stride, (int)position - (int)radius, length, channel);
                double right =
                    sample(input, base, stride, (int)position + (int)radius, length, channel);
                output[base + (size_t)position * stride + channel] =
                    (sums[channel] + edge * (left + right)) / amount;
                sums[channel] += right - sample(input, base, stride,
                                                (int)position - (int)radius + 1, length, channel);
            }
    }
}
/* Round a bounded color component without converting negative or overflowing floating values. */
unsigned char byte(double value)
{
    return value <= 0 ? 0 : value >= 255 ? 255 : (unsigned char)std::floor(value + 0.5);
}
} // namespace
/* Compute the centered fractional window support; widths below one are identity. */
unsigned fvm_blur_radius(double amount)
{
    return amount <= 1 ? 0 : (unsigned)std::ceil((amount - 1) / 2);
}
/* [GC:] Refresh source/receiver after public conversion, blur full padded input and publish once.
 */
FVM_Result fvm_blur_apply(FVM_Context *c, FVM_Object *self, const FVM_BitmapRegion &region,
                          const double *options, FVM_Value *out)
{
    FVM_BitmapState *target = fvm_bitmap_pixels(self), *source = fvm_bitmap_pixels(region.source);
    if (!target || !source || target->disposed || source->disposed)
    {
        fvm_value_number(out, -1);
        return FVM_OK;
    }
    for (unsigned i = 0; i < 4; ++i)
        if (!(region.box[i] >= -2147483648.0 && region.box[i] <= 2147483647.0))
            return FVM_OK;
    for (unsigned i = 0; i < 2; ++i)
        if (!(region.destination[i] >= -2147483648.0 && region.destination[i] <= 2147483647.0))
            return FVM_OK;
    unsigned quality = (unsigned)options[2];
    unsigned pad_x = fvm_blur_radius(options[0]) * quality;
    unsigned pad_y = fvm_blur_radius(options[1]) * quality;
    double sx = std::floor(region.box[0]), sy = std::floor(region.box[1]);
    double right = sx + std::floor(region.box[2]), bottom = sy + std::floor(region.box[3]);
    if (sx < 0)
        sx = 0;
    if (sy < 0)
        sy = 0;
    if (right > source->width)
        right = source->width;
    if (bottom > source->height)
        bottom = source->height;
    if (sx >= right || sy >= bottom)
        return FVM_OK;
    double mapped_x = std::floor(region.destination[0]) + sx - std::floor(region.box[0]);
    double mapped_y = std::floor(region.destination[1]) + sy - std::floor(region.box[1]);
    double dx = mapped_x - pad_x, dy = mapped_y - pad_y;
    double end_x = mapped_x + right - sx + pad_x;
    double end_y = mapped_y + bottom - sy + pad_y;
    if (dx < 0)
        dx = 0;
    if (dy < 0)
        dy = 0;
    if (end_x > target->width)
        end_x = target->width;
    if (end_y > target->height)
        end_y = target->height;
    if (dx >= end_x || dy >= end_y)
        return FVM_OK;
    unsigned output_width = (unsigned)(end_x - dx), output_height = (unsigned)(end_y - dy);
    unsigned width = source->width + 2 * pad_x, height = source->height + 2 * pad_y;
    size_t components = (size_t)width * height * 4;
    size_t output_bytes = (size_t)output_width * output_height * 4;
    size_t bytes = components * sizeof(double) * 2 + output_bytes;
    unsigned long limit = c->runtime->config.memory_limit_bytes;
    if (limit && bytes > limit)
        return FVM_OUT_OF_MEMORY;
    fvm_detail::vector<double> input(components), temporary(components);
    fvm_detail::vector<unsigned char> pixels(output_bytes);
    for (unsigned y = 0; y < source->height; ++y)
        for (unsigned x = 0; x < source->width; ++x)
        {
            const unsigned char *p = source->pixels.begin() + ((size_t)y * source->width + x) * 4;
            size_t index = ((size_t)(y + pad_y) * width + x + pad_x) * 4;
            for (unsigned channel = 0; channel < 3; ++channel)
                input[index + channel] = (double)p[channel] * p[3] / 255;
            input[index + 3] = p[3];
        }
    for (unsigned i = 0; i < quality; ++i)
    {
        pass(input, temporary, width, height, options[0], true);
        pass(temporary, input, width, height, options[1], false);
    }
    int origin_x = (int)(sx + dx - mapped_x + pad_x);
    int origin_y = (int)(sy + dy - mapped_y + pad_y);
    for (unsigned y = 0; y < output_height; ++y)
        for (unsigned x = 0; x < output_width; ++x)
        {
            const double *p = input.begin() + ((size_t)(origin_y + y) * width + origin_x + x) * 4;
            unsigned char *dest = pixels.begin() + ((size_t)y * output_width + x) * 4;
            double alpha = p[3];
            dest[3] = target->transparent ? byte(alpha) : 255;
            for (unsigned channel = 0; channel < 3; ++channel)
                dest[channel] = !dest[3] || alpha <= 0 ? 0 : byte(p[channel] * 255 / alpha);
        }
    /* [GC:] All buffers are complete; publication has no allocation or callback safe points. */
    for (unsigned y = 0; y < output_height; ++y)
        std::memcpy(target->pixels.begin() +
                        ((size_t)((unsigned)dy + y) * target->width + (unsigned)dx) * 4,
                    pixels.begin() + (size_t)y * output_width * 4, (size_t)output_width * 4);
    return FVM_OK;
}
