#define FVM_JPEG_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "image_codecs_swf_jpeg.cpp"
#include "../src/capi/swf_image_internal.h"
#include "png.h"
#include "gif_lib.h"
#include "image_codecs_memory.h"


static inline Bytes png_fixture()
{
    png_image image;
    std::memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;
    image.width = image.height = 8;
    image.format = PNG_FORMAT_RGBA;
    unsigned char pixels[256];
    for (unsigned i = 0; i < 64; ++i)
    {
        pixels[i * 4] = 64;
        pixels[i * 4 + 1] = 32;
        pixels[i * 4 + 2] = 16;
        pixels[i * 4 + 3] = 128;
    }
    pixels[3] = 0;
    png_alloc_size_t size = 0;
    if (!(png_image_write_to_memory(&image, 0, &size, 0, pixels, 0, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }


    Bytes output(size);
    if (!(png_image_write_to_memory(&image, output.begin(), &size, 0, pixels, 0, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    output.resize(size);
    return output;
}


static int gif_write(GifFileType *gif, const GifByteType *data, int count)
{
    TestImageBytes *output = (TestImageBytes *)gif->UserData;
    if (count < 0 || (size_t)count > sizeof(output->data) - output->size)
    {
        return 0;
    }
    std::memcpy(output->data + output->size, data, (size_t)count);
    output->size += (size_t)count;
    return count;
}


static void png_write_fixture(png_structp png, png_bytep data, png_size_t count)
{
    TestImageBytes *output = (TestImageBytes *)png_get_io_ptr(png);
    if (!(count <= sizeof(output->data) - output->size))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::memcpy(output->data + output->size, data, count);
    output->size += count;
}


static void png_flush_fixture(png_structp png)
{
    (void)png;
}


static inline Bytes png_layout_fixture(unsigned variant)
{
    TestImageBytes output;
    std::memset(&output, 0, sizeof(output));
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!(png))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    png_infop info = png_create_info_struct(png);
    if (!(info))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!setjmp(png_jmpbuf(png))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    png_set_write_fn(png, &output, png_write_fixture, png_flush_fixture);
    int type = variant == 0   ? PNG_COLOR_TYPE_PALETTE
               : variant == 1 ? PNG_COLOR_TYPE_GRAY
               : variant == 2 ? PNG_COLOR_TYPE_GRAY_ALPHA
                              : PNG_COLOR_TYPE_RGB;
    int depth = variant == 0 ? 1 : variant == 1 ? 2 : variant == 2 ? 16 : 8;
    png_set_IHDR(png,
                 info,
                 8,
                 8,
                 depth,
                 type,
                 variant == 3 ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_color palette[2];
    std::memset(palette, 0, sizeof(palette));
    palette[1].red = 255;
    png_byte alpha[2] = {0, 255};
    if (!variant)
    {
        png_set_PLTE(png, info, palette, 2);
        png_set_tRNS(png, info, alpha, 2, 0);
    }
    unsigned char pixels[8][32];
    std::memset(pixels, 0, sizeof(pixels));
    png_bytep rows[8];
    for (unsigned y = 0; y < 8; ++y)
    {
        rows[y] = pixels[y];
        for (unsigned x = 0; x < 8; ++x)
        {
            if (!variant)
            {
                pixels[y][0] = (unsigned char)(y % 2 ? 170 : 85);
            }
            else if (variant == 1)
            {
                pixels[y][x / 4] |= (unsigned char)(1 << (6 - 2 * (x % 4)));
            }
            else if (variant == 2)
            {
                pixels[y][x * 4] = pixels[y][x * 4 + 1] = 64;
                pixels[y][x * 4 + 2] = pixels[y][x * 4 + 3] = 128;
            }
            else
            {
                pixels[y][x * 3] = 64;
                pixels[y][x * 3 + 1] = 32;
                pixels[y][x * 3 + 2] = 16;
            }
        }
    }
    png_write_info(png, info);
    png_write_image(png, rows);
    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);
    Bytes result;
    for (size_t i = 0; i < output.size; ++i)
    {
        result.push_back(output.data[i]);
    }
    return result;
}


static inline Bytes gif_fixture(bool interlace = false, bool animation = false, bool local = false)
{
    TestImageBytes output;
    std::memset(&output, 0, sizeof(output));
    GifColorType colors[2];
    std::memset(colors, 0, sizeof(colors));
    colors[1].Red = 255;
    ColorMapObject *palette = GifMakeMapObject(2, colors);
    if (!(palette))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int error = 0;
    GifFileType *gif = EGifOpen(&output, gif_write, &error);
    if (!(gif))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    EGifSetGifVersion(gif, 1);
    if (!(EGifPutScreenDesc(gif, 8, 8, 8, 0, palette) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    GraphicsControlBlock control;
    control.DisposalMode = DISPOSE_DO_NOT;
    control.UserInputFlag = 0;
    control.DelayTime = 0;
    control.TransparentColor = 0;
    GifByteType extension[4];
    if (!(EGifGCBToExtension(&control, extension) == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(EGifPutExtension(gif, GRAPHICS_EXT_FUNC_CODE, 4, extension) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned frame = 0; frame < (animation ? 2U : 1U); ++frame)
    {
        if (!(EGifPutImageDesc(gif, 0, 0, 8, 8, interlace, (frame || local) ? palette : 0) == GIF_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned starts[] = {0, 4, 2, 1}, steps[] = {8, 8, 4, 2};
        GifPixelType row[8];
        for (unsigned pass = 0; pass < (interlace ? 4U : 1U); ++pass)
        {
            for (unsigned y = interlace ? starts[pass] : 0; y < 8; y += interlace ? steps[pass] : 1)
            {
                for (unsigned x = 0; x < 8; ++x)
                {
                    row[x] = (GifPixelType)((x + y) % 2);
                }
                if (!(EGifPutLine(gif, row, 8) == GIF_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    if (!(EGifCloseFile(gif, &error) == GIF_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    GifFreeMapObject(palette);
    Bytes bytes;
    for (size_t i = 0; i < output.size; ++i)
    {
        bytes.push_back(output.data[i]);
    }
    return bytes;
}


static inline void png_gif_pixels(FVM_Context *c, FVM_Object *root, bool gif)
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
    if (!(pixels[3] == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (gif)
    {
        if (!(pixels[4] == 255 && pixels[7] == 255))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(pixels[4 * 16] == 255 && pixels[4 * 16 + 3] == 255))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(pixels[4 * (16 + 1) + 3] == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(pixels[4] == 64 && pixels[5] == 32 && pixels[6] == 16 && pixels[7] == 128))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}
#ifndef FVM_PNG_GIF_FIXTURE_ONLY


int main()
{
    Bytes sources[3] = {png_fixture(), gif_fixture(), gif_fixture(true)};
    for (unsigned variant = 0; variant < 3; ++variant)
    {
        unsigned width = 0, height = 0;
        unsigned char *pixels = 0;
        bool gif = variant != 0;
        Bytes &source = sources[variant];
        if (!((gif ? fvm_swf_decode_gif(source.begin(), source.size(), 1024, &width, &height, &pixels)
                   : fvm_swf_decode_png(source.begin(), source.size(), 1024, &width, &height, &pixels)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(width == 8 && height == 8))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (gif)
        {
            if (!(pixels[3] == 0 && pixels[4] == 255 && pixels[7] == 255))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(pixels[32] == 255 && pixels[35] == 255))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(pixels[4] == 64 && pixels[7] == 128))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        std::free(pixels);
        for (size_t n = 0; n < source.size(); ++n)
        {
            width = height = 99;
            pixels = (unsigned char *)1;
            FVM_Result r = gif ? fvm_swf_decode_gif(source.begin(), n, 1024, &width, &height, &pixels)
                               : fvm_swf_decode_png(source.begin(), n, 1024, &width, &height, &pixels);
            if (!(r == FVM_INVALID_ARGUMENT))
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
        Bytes extra = source;
        extra.push_back(0);
        if (!((gif ? fvm_swf_decode_gif(extra.begin(), extra.size(), 1024, &width, &height, &pixels)
                   : fvm_swf_decode_png(extra.begin(), extra.size(), 1024, &width, &height, &pixels)) ==
              FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!((gif ? fvm_swf_decode_gif(source.begin(), source.size(), 255, &width, &height, &pixels)
                   : fvm_swf_decode_png(source.begin(), source.size(), 255, &width, &height, &pixels)) ==
              FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_SWFBitmap decoded;
        Bytes payload = jpeg_resource(source);
        if (!(fvm_swf_read_jpeg(payload.begin(), payload.size(), 21, 8, 1024, decoded) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_swf_read_jpeg(payload.begin(), payload.size(), 21, 7, 1024, decoded) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes jpeg3;
        u16(jpeg3, 7);
        u32(jpeg3, (unsigned long)source.size());
        append(jpeg3, source);
        if (!(fvm_swf_read_jpeg(jpeg3.begin(), jpeg3.size(), 35, 8, 1024, decoded) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        jpeg3.push_back(0);
        if (!(fvm_swf_read_jpeg(jpeg3.begin(), jpeg3.size(), 35, 8, 1024, decoded) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned variant = 0; variant < 4; ++variant)
    {
        Bytes layout = png_layout_fixture(variant);
        unsigned width, height;
        unsigned char *pixels = 0;
        if (!(fvm_swf_decode_png(layout.begin(), layout.size(), 1024, &width, &height, &pixels) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(width == 8 && height == 8))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!variant)
        {
            if (!(pixels[3] == 0 && pixels[4] == 255 && pixels[7] == 255))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(pixels[32] == 255 && pixels[35] == 255))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (variant == 1)
        {
            if (!(pixels[0] == 85 && pixels[1] == 85 && pixels[2] == 85 && pixels[3] == 255))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (variant == 2)
        {
            if (!(pixels[0] == 64 && pixels[1] == 64 && pixels[2] == 64 && pixels[3] == 128))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(pixels[0] == 64 && pixels[1] == 32 && pixels[2] == 16 && pixels[3] == 255))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        std::free(pixels);
    }
    Bytes local = gif_fixture(true, false, true);
    unsigned local_w, local_h;
    unsigned char *local_pixels = 0;
    if (!(fvm_swf_decode_gif(local.begin(), local.size(), 1024, &local_w, &local_h, &local_pixels) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(local_pixels[4] == 255 && local_pixels[7] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::free(local_pixels);
    Bytes huge = sources[1];
    bool huge_patched = false;
    for (size_t i = 13; i + 9 < huge.size(); ++i)
    {
        if (huge[i] == 44)
        {
            huge[i + 5] = huge[i + 6] = huge[i + 7] = huge[i + 8] = 255;
            huge_patched = true;
            break;
        }
    }
    if (!(huge_patched))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_decode_gif(huge.begin(), huge.size(), 1024, &local_w, &local_h, &local_pixels) == FVM_UNSUPPORTED &&
          !local_pixels))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes animated = gif_fixture(false, true);
    unsigned w, h;
    unsigned char *p = 0;
    if (!(fvm_swf_decode_gif(animated.begin(), animated.size(), 1024, &w, &h, &p) == FVM_UNSUPPORTED && !p))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes corrupt = sources[0];
    corrupt[29] ^= 1;
    if (!(fvm_swf_decode_png(corrupt.begin(), corrupt.size(), 1024, &w, &h, &p) == FVM_INVALID_ARGUMENT && !p))
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
    for (unsigned variant = 0; variant < 3; ++variant)
    {
        for (unsigned tag_code = 21; tag_code <= 35; tag_code += 14)
        {
            Bytes tags, empty, payload;
            u16(payload, 7);
            if (tag_code == 35)
            {
                u32(payload, (unsigned long)sources[variant].size());
            }
            append(payload, sources[variant]);
            tag(tags, tag_code, payload);
            tag(tags, 32, bitmap_shape(0x43));
            tag(tags, 26, place(2 | 4, 1, 9, 0, 0));
            tag(tags, 1, empty);
            tag(tags, 0, empty);
            FVM_Object *root = load(c, tags, 1);
            tick(c, root);
            png_gif_pixels(c, root, variant != 0);
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            png_gif_pixels(c, root, variant != 0);
            FVM_Channel *ch = 0;
            FVM_Value value, out;
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = root;
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
            png_gif_pixels(other, out.object, variant != 0);
            fvm_channel_destroy(ch);
        }
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
