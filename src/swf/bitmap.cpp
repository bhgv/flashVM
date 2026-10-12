/* FlashVM: strict size-bounded lossless SWF bitmap decoding with palette/15-bit/ARGB layouts. */
#include "bitmap.hpp"
#include "zlib.h"
#include "../capi/swf_jpeg_internal.h"
#include "../capi/swf_image_internal.h"
#include <cstdlib>
#include <cstring>
#include <climits>
#include <new>

namespace
{
/* Decode a little-endian unsigned header word without unaligned native loads. */
unsigned word(const unsigned char *bytes)
{
    return bytes[0] | ((unsigned)bytes[1] << 8);
}
/* Recover one straight channel from a premultiplied byte, clamping malformed over-alpha data. */
unsigned char straight(unsigned color, unsigned alpha)
{
    if (!alpha)
        return 0;
    unsigned value = (color * 255 + alpha / 2) / alpha;
    return (unsigned char)(value > 255 ? 255 : value);
}
} // namespace
/* Decode a complete zlib stream and exact padded payload before publishing straight RGBA. */
FVM_Result fvm_swf_read_bitmap(const unsigned char *bytes, size_t size, unsigned tag, size_t limit,
                               FVM_SWFBitmap &out)
{
    if (!bytes || size < 7 || (tag != 20 && tag != 36))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFBitmap result;
    result.id = word(bytes);
    unsigned format = bytes[2];
    result.width = word(bytes + 3);
    result.height = word(bytes + 5);
    result.transparent = tag == 36;
    if (!result.id || !result.width || !result.height ||
        (format != 3 && format != 4 && format != 5) || (tag == 36 && format == 4))
        return FVM_INVALID_ARGUMENT;
    if (result.width > 2880 || result.height > 2880)
        return FVM_UNSUPPORTED;
    size_t header = format == 3 ? 8 : 7;
    if (size <= header || size - header > ULONG_MAX)
        return FVM_INVALID_ARGUMENT;
    unsigned colors = format == 3 ? (unsigned)bytes[7] + 1 : 0;
    size_t palette = (size_t)colors * (result.transparent ? 4 : 3);
    unsigned bytes_per_pixel = format == 3 ? 1 : format == 4 ? 2 : 4;
    size_t stride = ((size_t)result.width * bytes_per_pixel + 3) & ~(size_t)3;
    size_t expected = palette + stride * result.height,
           pixel_bytes = (size_t)result.width * result.height * 4;
    if ((limit && (expected > limit || pixel_bytes > limit)) || expected > ULONG_MAX)
        return FVM_OUT_OF_MEMORY;
    try
    {
        fvm_detail::vector<unsigned char> data(expected);
        uLongf decoded = (uLongf)expected;
        uLong consumed = (uLong)(size - header);
        int status = uncompress2(data.begin(), &decoded, bytes + header, &consumed);
        if (status == Z_MEM_ERROR)
            return FVM_OUT_OF_MEMORY;
        if (status != Z_OK || decoded != expected || consumed != size - header)
            return FVM_INVALID_ARGUMENT;
        result.pixels.resize(pixel_bytes);
        for (unsigned y = 0; y < result.height; ++y)
            for (unsigned x = 0; x < result.width; ++x)
            {
                const unsigned char *source =
                    data.begin() + palette + y * stride + x * bytes_per_pixel;
                unsigned char *dest = result.pixels.begin() + ((size_t)y * result.width + x) * 4;
                if (format == 3)
                {
                    unsigned index = source[0];
                    if (index >= colors)
                        return FVM_INVALID_ARGUMENT;
                    source = data.begin() + index * (result.transparent ? 4 : 3);
                    dest[3] = result.transparent ? source[3] : 255;
                    for (unsigned channel = 0; channel < 3; ++channel)
                        dest[channel] = result.transparent ? straight(source[channel], dest[3])
                                                           : source[channel];
                }
                else if (format == 4)
                {
                    unsigned value = ((unsigned)source[0] << 8) | source[1];
                    if (value & 0x8000)
                        return FVM_INVALID_ARGUMENT;
                    dest[0] = (unsigned char)((((value >> 10) & 31) * 255 + 15) / 31);
                    dest[1] = (unsigned char)((((value >> 5) & 31) * 255 + 15) / 31);
                    dest[2] = (unsigned char)(((value & 31) * 255 + 15) / 31);
                    dest[3] = 255;
                }
                else
                {
                    if (!result.transparent && source[0])
                        return FVM_INVALID_ARGUMENT;
                    dest[3] = result.transparent ? source[0] : 255;
                    for (unsigned channel = 0; channel < 3; ++channel)
                        dest[channel] = result.transparent ? straight(source[channel + 1], dest[3])
                                                           : source[channel + 1];
                }
            }
        out.id = result.id;
        out.width = result.width;
        out.height = result.height;
        out.transparent = result.transparent;
        out.pixels.swap(result.pixels);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Decode a bounded JPEG and optional premultiplied alpha without publishing partial resources. */
FVM_Result fvm_swf_read_jpeg(const unsigned char *bytes, size_t size, unsigned tag,
                             unsigned version, size_t limit, FVM_SWFBitmap &out)
{
    if (!bytes || (tag != 21 && tag != 35) || size <= (tag == 35 ? 6U : 2U) ||
        version < (tag == 35 ? 3U : 2U) || !word(bytes))
        return FVM_INVALID_ARGUMENT;
    size_t header = tag == 35 ? 6 : 2;
    size_t image_size = tag == 35 ? (size_t)bytes[2] | ((size_t)bytes[3] << 8) |
                                        ((size_t)bytes[4] << 16) | ((size_t)bytes[5] << 24)
                                  : size - header;
    if (!image_size || image_size > size - header)
        return FVM_INVALID_ARGUMENT;
    size_t alpha_at = header + image_size;
    const unsigned char *image = bytes + header;
    const unsigned char legacy[] = {255, 217, 255, 216};
    if (version < 8 && image_size >= 6 && !std::memcmp(image, legacy, sizeof(legacy)))
    {
        image += 4;
        image_size -= 4;
    }
    bool png = image_size >= 8 && !std::memcmp(image, "\211PNG\r\n\032\n", 8);
    bool gif = image_size >= 6 && !std::memcmp(image, "GIF89a", 6);
    if ((png || gif) && (version < 8 || (tag == 35 && alpha_at != size)))
        return FVM_INVALID_ARGUMENT;
    FVM_SWFBitmap result;
    unsigned char *pixels = 0;
    FVM_Result status =
        png ? fvm_swf_decode_png(image, image_size, limit, &result.width, &result.height, &pixels)
        : gif
            ? fvm_swf_decode_gif(image, image_size, limit, &result.width, &result.height, &pixels)
            : fvm_swf_decode_jpeg(image, image_size, limit, &result.width, &result.height, &pixels);
    if (status != FVM_OK)
        return status;
    try
    {
        result.id = word(bytes);
        result.transparent = tag == 35 || png || gif;
        size_t count = (size_t)result.width * result.height;
        result.pixels.resize(count * 4);
        std::memcpy(result.pixels.begin(), pixels, count * 4);
        std::free(pixels);
        pixels = 0;
        if (tag == 35 && !png && !gif)
        {
            if (alpha_at >= size || size - alpha_at > ULONG_MAX)
                return FVM_INVALID_ARGUMENT;
            fvm_detail::vector<unsigned char> alpha(count);
            uLongf decoded = (uLongf)count;
            uLong consumed = (uLong)(size - alpha_at);
            int z = uncompress2(alpha.begin(), &decoded, bytes + alpha_at, &consumed);
            if (z == Z_MEM_ERROR)
                return FVM_OUT_OF_MEMORY;
            if (z != Z_OK || decoded != count || consumed != size - alpha_at)
                return FVM_INVALID_ARGUMENT;
            for (size_t i = 0; i < count; ++i)
            {
                unsigned char *p = result.pixels.begin() + i * 4;
                p[3] = alpha[i];
                for (unsigned j = 0; j < 3; ++j)
                    p[j] = straight(p[j], p[3]);
            }
        }
        out.id = result.id;
        out.width = result.width;
        out.height = result.height;
        out.transparent = result.transparent;
        out.pixels.swap(result.pixels);
        return FVM_OK;
    }
    catch (const std::bad_alloc &)
    {
        std::free(pixels);
        return FVM_OUT_OF_MEMORY;
    }
}
