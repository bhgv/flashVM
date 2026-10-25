#define FVM_BITMAP_SWF_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "swf_bitmap_fill.cpp"
#include "../src/capi/swf_jpeg_internal.h"
#include <cstdlib>
#include "jpeglib.h"


static inline Bytes jpeg_fixture(bool grayscale = false, bool progressive = false)
{
    jpeg_compress_struct encoder;
    jpeg_error_mgr error;
    std::memset(&encoder, 0, sizeof(encoder));
    encoder.err = jpeg_std_error(&error);
    unsigned char *encoded = 0;
    unsigned long size = 0;
    jpeg_create_compress(&encoder);
    jpeg_mem_dest(&encoder, &encoded, &size);
    encoder.image_width = encoder.image_height = 8;
    encoder.input_components = grayscale ? 1 : 3;
    encoder.in_color_space = grayscale ? JCS_GRAYSCALE : JCS_RGB;
    jpeg_set_defaults(&encoder);
    jpeg_set_quality(&encoder, 100, TRUE);
    for (int i = 0; i < encoder.num_components; ++i)
    {
        encoder.comp_info[i].h_samp_factor = encoder.comp_info[i].v_samp_factor = 1;
    }
    if (progressive)
    {
        jpeg_simple_progression(&encoder);
    }
    jpeg_start_compress(&encoder, TRUE);
    unsigned char row[24];
    for (unsigned i = 0; i < 8; ++i)
    {
        if (grayscale)
        {
            row[i] = 64;
        }
        else
        {
            row[i * 3] = 64;
            row[i * 3 + 1] = 32;
            row[i * 3 + 2] = 16;
        }
    }
    JSAMPROW rows[1] = {row};
    while (encoder.next_scanline < encoder.image_height)
    {
        if (!(jpeg_write_scanlines(&encoder, rows, 1) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    jpeg_finish_compress(&encoder);
    Bytes output;
    for (unsigned long i = 0; i < size; ++i)
    {
        output.push_back(encoded[i]);
    }
    jpeg_destroy_compress(&encoder);
    std::free(encoded);
    return output;
}


static inline Bytes jpeg_resource(const Bytes &image, bool alpha = false)
{
    Bytes result;
    u16(result, 7);
    if (alpha)
    {
        u32(result, (unsigned long)image.size());
    }
    append(result, image);
    if (alpha)
    {


        Bytes raw(64);
        for (unsigned i = 0; i < 64; ++i)
        {
            raw[i] = 128;
        }
        raw[0] = 0;
        raw[63] = 255;
        uLongf size = compressBound(64);


        Bytes packed(size);
        if (!(compress2(packed.begin(), &size, raw.begin(), 64, Z_BEST_COMPRESSION) == Z_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        packed.resize(size);
        append(result, packed);
    }
    return result;
}


static inline Bytes jpeg_movie(bool alpha = false)
{
    Bytes tags, empty;
    tag(tags, alpha ? 35 : 21, jpeg_resource(jpeg_fixture(), alpha));
    tag(tags, 32, bitmap_shape(0x43));
    tag(tags, 26, place(2 | 4, 1, 9, 0, 0));
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    return movie(tags, 1);
}


static inline void jpeg_pixels(FVM_Context *c, FVM_Object *root, bool alpha)
{
    unsigned char pixels[16 * 16 * 4];
    std::memset(pixels, 0, sizeof(pixels));
    FVM_RasterSurface surface = {pixels, 16, 16, 64};
    FVM_Result r = fvm_movieclip_render(c, root, &surface);
    if (r == FVM_UNSUPPORTED)
    {
        return;
    }
    if (!(r == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char *p = pixels + 4 * (4 * 16 + 4);
    if (!(p[0] >= (alpha ? 126 : 62) && p[0] <= (alpha ? 130 : 66)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(p[1] >= (alpha ? 62 : 30) && p[1] <= (alpha ? 66 : 34)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(p[2] >= (alpha ? 30 : 14) && p[2] <= (alpha ? 34 : 18)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(p[3] == (alpha ? 128 : 255)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (alpha)
    {
        if (!(pixels[0] == 0 && pixels[1] == 0 && pixels[2] == 0 && pixels[3] == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}
#ifndef FVM_JPEG_FIXTURE_ONLY


int main()
{
    Bytes jpeg = jpeg_fixture(), source = jpeg_resource(jpeg);
    FVM_SWFBitmap decoded;
    if (!(fvm_swf_read_jpeg(source.begin(), source.size(), 21, 8, 1024, decoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(decoded.width == 8 && decoded.height == 8 && !decoded.transparent))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t n = 0; n < jpeg.size(); ++n)
    {
        unsigned width = 99, height = 99;
        unsigned char *pixels = (unsigned char *)1;
        if (!(fvm_swf_decode_jpeg(jpeg.begin(), n, 1024, &width, &height, &pixels) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!width && !height && !pixels))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_swf_read_jpeg(source.begin(), source.size(), 21, 1, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes huge = jpeg;
    bool patched = false;
    for (size_t i = 0; i + 8 < huge.size(); ++i)
    {
        if (huge[i] == 255 && huge[i + 1] == 192)
        {
            huge[i + 7] = 11;
            huge[i + 8] = 65;
            patched = true;
            break;
        }
    }
    if (!(patched))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    huge = jpeg_resource(huge);
    if (!(fvm_swf_read_jpeg(huge.begin(), huge.size(), 21, 8, 1024, decoded) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes extra = source;
    extra.push_back(0);
    if (!(fvm_swf_read_jpeg(extra.begin(), extra.size(), 21, 8, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_read_jpeg(source.begin(), source.size(), 21, 8, 255, decoded) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(decoded.id == 7 && decoded.pixels.size() == 256))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes gray = jpeg_resource(jpeg_fixture(true));
    if (!(fvm_swf_read_jpeg(gray.begin(), gray.size(), 21, 8, 1024, decoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(decoded.pixels[0] == decoded.pixels[1] && decoded.pixels[1] == decoded.pixels[2]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes legacy;
    u16(legacy, 7);
    legacy.push_back(255);
    legacy.push_back(217);
    legacy.push_back(255);
    legacy.push_back(216);
    append(legacy, jpeg);
    if (!(fvm_swf_read_jpeg(legacy.begin(), legacy.size(), 21, 7, 1024, decoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_read_jpeg(legacy.begin(), legacy.size(), 21, 8, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    source = jpeg_resource(jpeg, true);
    if (!(fvm_swf_read_jpeg(source.begin(), source.size(), 35, 8, 1024, decoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rgba(decoded, 0, 0, 0, 0, 0);
    if (!(decoded.transparent && decoded.pixels[4] >= 126 && decoded.pixels[4] <= 130 && decoded.pixels[7] == 128))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_read_jpeg(source.begin(), source.size(), 35, 2, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    extra = source;
    extra.push_back(0);
    if (!(fvm_swf_read_jpeg(extra.begin(), extra.size(), 35, 8, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t n = 6 + jpeg.size(); n < source.size(); ++n)
    {
        if (!(fvm_swf_read_jpeg(source.begin(), n, 35, 8, 1024, decoded) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    extra = source;
    extra[2] = extra[3] = extra[4] = extra[5] = 255;
    if (!(fvm_swf_read_jpeg(extra.begin(), extra.size(), 35, 8, 1024, decoded) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned a = 0; a < 2; ++a)
    {
        Bytes swf = jpeg_movie(a != 0);
        FVM_Object *root = 0;
        if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick(c, root);
        jpeg_pixels(c, root, a != 0);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        jpeg_pixels(c, root, a != 0);
        FVM_Value value, out;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = root;
        FVM_Channel *ch = 0;
        if (!(fvm_channel_create(&ch) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_receive_value(other, ch, &out, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(other, out.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        jpeg_pixels(other, out.object, a != 0);
        fvm_channel_destroy(ch);
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
