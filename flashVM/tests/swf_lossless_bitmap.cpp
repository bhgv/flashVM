#define FVM_SHAPE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "swf_shape.cpp"
#include "../src/swf/bitmap.hpp"
#include "zlib.h"


static inline Bytes
lossless(unsigned format, unsigned width, unsigned height, const Bytes &raw, unsigned colors = 0, unsigned id = 7)
{
    Bytes result;
    u16(result, id);
    result.push_back((unsigned char)format);
    u16(result, width);
    u16(result, height);
    if (format == 3)
    {
        result.push_back((unsigned char)(colors - 1));
    }
    uLongf size = compressBound((uLong)raw.size());


    Bytes compressed(size);
    if (!(compress2(compressed.begin(), &size, raw.begin(), (uLong)raw.size(), Z_BEST_COMPRESSION) == Z_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    compressed.resize(size);
    append(result, compressed);
    return result;
}


static inline void rgba(const FVM_SWFBitmap &bitmap, unsigned x, unsigned r, unsigned g, unsigned b, unsigned a)
{
    if (!(bitmap.pixels[x * 4] == r && bitmap.pixels[x * 4 + 1] == g && bitmap.pixels[x * 4 + 2] == b &&
          bitmap.pixels[x * 4 + 3] == a))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static inline Bytes bitmap_raw()
{
    const unsigned char data[] = {0, 255, 0, 0, 0, 0, 255, 0};
    Bytes raw;
    for (unsigned i = 0; i < sizeof(data); ++i)
    {
        raw.push_back(data[i]);
    }
    return raw;
}
#ifndef FVM_LOSSLESS_FIXTURE_ONLY


int main()
{
    FVM_SWFBitmap decoded;
    Bytes raw = bitmap_raw(), source = lossless(5, 2, 1, raw);
    if (!(fvm_swf_read_bitmap(source.begin(), source.size(), 20, 1024, decoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(decoded.id == 7 && decoded.width == 2 && decoded.height == 1 && !decoded.transparent))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rgba(decoded, 0, 255, 0, 0, 255);
    rgba(decoded, 1, 0, 255, 0, 255);
    for (size_t n = 0; n < source.size(); ++n)
    {
        if (!(fvm_swf_read_bitmap(source.begin(), n, 20, 1024, decoded) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(decoded.id == 7 && decoded.pixels.size() == 8))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rgba(decoded, 0, 255, 0, 0, 255);
    }
    Bytes extra = source;
    extra.push_back(0);
    if (!(fvm_swf_read_bitmap(extra.begin(), extra.size(), 20, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_read_bitmap(source.begin(), source.size(), 20, 7, decoded) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes invalid = source;
    invalid[0] = invalid[1] = 0;
    if (!(fvm_swf_read_bitmap(invalid.begin(), invalid.size(), 20, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    invalid = source;
    invalid[3] = invalid[4] = 255;
    if (!(fvm_swf_read_bitmap(invalid.begin(), invalid.size(), 20, 1024, decoded) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    raw[0] = 1;
    invalid = lossless(5, 2, 1, raw);
    if (!(fvm_swf_read_bitmap(invalid.begin(), invalid.size(), 20, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    raw = bitmap_raw();
    raw[0] = 128;
    raw[1] = 64;
    raw[4] = 0;
    raw[5] = 123;
    source = lossless(5, 2, 1, raw);
    if (!(fvm_swf_read_bitmap(source.begin(), source.size(), 36, 1024, decoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rgba(decoded, 0, 128, 0, 0, 128);
    rgba(decoded, 1, 0, 0, 0, 0);
    raw.clear();
    raw.push_back(0x7c);
    raw.push_back(0);
    raw.push_back(0);
    raw.push_back(0x1f);
    source = lossless(4, 2, 1, raw);
    if (!(fvm_swf_read_bitmap(source.begin(), source.size(), 20, 1024, decoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rgba(decoded, 0, 255, 0, 0, 255);
    rgba(decoded, 1, 0, 0, 255, 255);
    if (!(fvm_swf_read_bitmap(source.begin(), source.size(), 36, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    raw[0] |= 128;
    invalid = lossless(4, 2, 1, raw);
    if (!(fvm_swf_read_bitmap(invalid.begin(), invalid.size(), 20, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    raw.clear();
    const unsigned char palette[] = {255, 0, 0, 0, 255, 0, 1, 0, 71, 83};
    for (unsigned i = 0; i < sizeof(palette); ++i)
    {
        raw.push_back(palette[i]);
    }
    source = lossless(3, 2, 1, raw, 2);
    if (!(fvm_swf_read_bitmap(source.begin(), source.size(), 20, 1024, decoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rgba(decoded, 0, 0, 255, 0, 255);
    rgba(decoded, 1, 255, 0, 0, 255);
    raw[6] = 2;
    invalid = lossless(3, 2, 1, raw, 2);
    if (!(fvm_swf_read_bitmap(invalid.begin(), invalid.size(), 20, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    raw.clear();
    const unsigned char alpha_palette[] = {64, 0, 0, 128, 0, 0, 0, 0, 0, 1, 99, 123};
    for (unsigned i = 0; i < sizeof(alpha_palette); ++i)
    {
        raw.push_back(alpha_palette[i]);
    }
    source = lossless(3, 2, 1, raw, 2);
    if (!(fvm_swf_read_bitmap(source.begin(), source.size(), 36, 1024, decoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rgba(decoded, 0, 128, 0, 0, 128);
    rgba(decoded, 1, 0, 0, 0, 0);
    raw.push_back(0);
    invalid = lossless(3, 2, 1, raw, 2);
    if (!(fvm_swf_read_bitmap(invalid.begin(), invalid.size(), 36, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    raw.resize(raw.size() - 2);
    invalid = lossless(3, 2, 1, raw, 2);
    if (!(fvm_swf_read_bitmap(invalid.begin(), invalid.size(), 36, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
#endif
