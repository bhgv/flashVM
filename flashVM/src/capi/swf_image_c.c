/* FlashVM: bounded PNG and single-frame GIF decoding with C-only library error handling. */
#include "swf_image_internal.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef FVM_HAS_IMAGE_CODECS
#include <setjmp.h>
#include "png.h"
#include "gif_lib.h"
#include "zlib.h"


/* One bounded input cursor shared by the PNG and GIF C readers. */
typedef struct FVM_ImageInput
{
    const unsigned char *bytes; /* Borrowed immutable input for this decoding call. */
    size_t size;                /* Total number of available input bytes. */
    size_t position;            /* Next byte consumed by a library reader. */
} FVM_ImageInput;


/* Mutable PNG state lives in heap so every cleanup field survives longjmp. */
typedef struct FVM_PngDecode
{
    png_structp png;       /* Read-side libpng state, including its allocation pools. */
    png_infop info;        /* Owned image metadata. */
    jmp_buf jump;          /* C-only error recovery point. */
    FVM_ImageInput input;  /* Exact bounded memory stream. */
    FVM_Result result;     /* Allocation-specific or malformed-input error. */
    unsigned char *pixels; /* Unpublished tightly packed RGBA allocation. */
    png_bytep *rows;       /* Row views into pixels, owned until cleanup. */
} FVM_PngDecode;


/* Read a big-endian PNG word without unaligned loads or native-endian assumptions. */
static unsigned long png_word(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) | ((unsigned long)p[2] << 8) | p[3];
}


/* Validate complete PNG chunk boundaries and CRCs before any image allocation. */
static FVM_Result png_chunks(const unsigned char *bytes, size_t size)
{
    size_t at = 8;
    while (at < size)
    {
        size_t length;
        uLong checksum;
        if (size - at < 12)
        {
            return FVM_INVALID_ARGUMENT;
        }
        length = (size_t)png_word(bytes + at);
        if (length > size - at - 12 || length > UINT_MAX - 4U)
        {
            return FVM_INVALID_ARGUMENT;
        }
        checksum = crc32(0, bytes + at + 4, (uInt)(length + 4));
        if (checksum != png_word(bytes + at + 8 + length))
        {
            return FVM_INVALID_ARGUMENT;
        }
        if (!memcmp(bytes + at + 4, "acTL", 4) || !memcmp(bytes + at + 4, "fcTL", 4) ||
            !memcmp(bytes + at + 4, "fdAT", 4))
        {
            return FVM_UNSUPPORTED;
        }
        if (!memcmp(bytes + at + 4, "IEND", 4))
        {
            return !length && at + 12 == size ? FVM_OK : FVM_INVALID_ARGUMENT;
        }
        at += length + 12;
    }
    return FVM_INVALID_ARGUMENT;
}


/* Allocate through libpng's custom C allocator and retain an exact OOM result on failure. */
static png_voidp png_allocate(png_structp png, png_alloc_size_t size)
{
    FVM_PngDecode *state = (FVM_PngDecode *)png_get_mem_ptr(png);
    void *memory = malloc((size_t)size);
    if (!memory)
    {
        state->result = FVM_OUT_OF_MEMORY;
    }
    return memory;
}


/* Free one libpng allocation through the matching C allocator. */
static void png_deallocate(png_structp png, png_voidp memory)
{
    (void)png;
    free(memory);
}


/* Return PNG failures entirely inside C without unwinding any C++ objects. */
static void png_failure(png_structp png, png_const_charp message)
{
    FVM_PngDecode *state = (FVM_PngDecode *)png_get_error_ptr(png);
    (void)message;
    longjmp(state->jump, 1);
}


/* Supply bytes from the bounded memory stream or signal an exact input underflow. */
static void png_input(png_structp png, png_bytep dest, png_size_t count)
{
    FVM_PngDecode *state = (FVM_PngDecode *)png_get_io_ptr(png);
    if (count > state->input.size - state->input.position)
    {
        png_error(png, "Truncated FlashVM PNG stream");
    }
    memcpy(dest, state->input.bytes + state->input.position, count);
    state->input.position += count;
}


/* Destroy native PNG metadata and every unpublished output allocation. */
static void png_cleanup(FVM_PngDecode *state)
{
    if (state->png)
    {
        png_destroy_read_struct(&state->png, &state->info, NULL);
    }
    free(state->rows);
    free(state->pixels);
    free(state);
}


/* Read at most the available GIF bytes without seeking or exposing unbounded memory. */
static int gif_input(GifFileType *gif, GifByteType *dest, int count)
{
    FVM_ImageInput *input = (FVM_ImageInput *)gif->UserData;
    size_t available = input->size - input->position;
    if (count < 0)
    {
        return 0;
    }
    if ((size_t)count > available)
    {
        count = (int)available;
    }
    memcpy(dest, input->bytes + input->position, (size_t)count);
    input->position += (size_t)count;
    return count;
}


/* Preserve giflib's allocation error while treating other decoder failures as malformed input. */
static FVM_Result gif_result(int error)
{
    return error == D_GIF_ERR_NOT_ENOUGH_MEM ? FVM_OUT_OF_MEMORY : FVM_INVALID_ARGUMENT;
}
#endif


/* Decode bounded PNG formats into straight RGBA while preserving alpha and exact stream length. */
FVM_Result fvm_swf_decode_png(
    const unsigned char *bytes, size_t size, size_t limit, unsigned *width, unsigned *height, unsigned char **pixels)
{
    if (!width || !height || !pixels)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *width = *height = 0;
    *pixels = NULL;
    if (!bytes || size < 33 || memcmp(bytes, "\211PNG\r\n\032\n", 8))
    {
        return FVM_INVALID_ARGUMENT;
    }
#ifdef FVM_HAS_IMAGE_CODECS
    {
        FVM_PngDecode *state;
        FVM_Result result;
        unsigned w, h, y;
        int type, depth;
        size_t pixel_bytes;
        if (png_word(bytes + 8) != 13 || memcmp(bytes + 12, "IHDR", 4))
        {
            return FVM_INVALID_ARGUMENT;
        }
        if (!png_word(bytes + 16) || !png_word(bytes + 20))
        {
            return FVM_INVALID_ARGUMENT;
        }
        if (png_word(bytes + 16) > 2880 || png_word(bytes + 20) > 2880)
        {
            return FVM_UNSUPPORTED;
        }
        w = (unsigned)png_word(bytes + 16);
        h = (unsigned)png_word(bytes + 20);
        pixel_bytes = (size_t)w * h * 4;
        if (limit && pixel_bytes > limit)
        {
            return FVM_OUT_OF_MEMORY;
        }
        result = png_chunks(bytes, size);
        if (result != FVM_OK)
        {
            return result;
        }
        state = (FVM_PngDecode *)calloc(1, sizeof(*state));
        if (!state)
        {
            return FVM_OUT_OF_MEMORY;
        }
        state->result = FVM_INVALID_ARGUMENT;
        state->input.bytes = bytes;
        state->input.size = size;
        if (setjmp(state->jump))
        {
            result = state->result;
            png_cleanup(state);
            return result;
        }
        state->png = png_create_read_struct_2(
            PNG_LIBPNG_VER_STRING, state, png_failure, png_failure, state, png_allocate, png_deallocate);
        if (!state->png)
        {
            png_cleanup(state);
            return FVM_OUT_OF_MEMORY;
        }
        state->info = png_create_info_struct(state->png);
        if (!state->info)
        {
            png_cleanup(state);
            return FVM_OUT_OF_MEMORY;
        }
        png_set_read_fn(state->png, state, png_input);
        png_set_crc_action(state->png, PNG_CRC_ERROR_QUIT, PNG_CRC_ERROR_QUIT);
        png_read_info(state->png, state->info);
        type = png_get_color_type(state->png, state->info);
        depth = png_get_bit_depth(state->png, state->info);
        if (depth == 16)
        {
            png_set_strip_16(state->png);
        }
        if (type == PNG_COLOR_TYPE_PALETTE)
        {
            png_set_palette_to_rgb(state->png);
        }
        if (type == PNG_COLOR_TYPE_GRAY && depth < 8)
        {
            png_set_expand_gray_1_2_4_to_8(state->png);
        }
        if (png_get_valid(state->png, state->info, PNG_INFO_tRNS))
        {
            png_set_tRNS_to_alpha(state->png);
        }
        else if (!(type & PNG_COLOR_MASK_ALPHA))
        {
            png_set_filler(state->png, 255, PNG_FILLER_AFTER);
        }
        if (type == PNG_COLOR_TYPE_GRAY || type == PNG_COLOR_TYPE_GRAY_ALPHA)
        {
            png_set_gray_to_rgb(state->png);
        }
        (void)png_set_interlace_handling(state->png);
        png_read_update_info(state->png, state->info);
        if (png_get_rowbytes(state->png, state->info) != (png_size_t)w * 4 ||
            png_get_channels(state->png, state->info) != 4)
        {
            png_error(state->png, "Unsupported FlashVM PNG row layout");
        }
        state->pixels = (unsigned char *)malloc(pixel_bytes);
        state->rows = (png_bytep *)malloc((size_t)h * sizeof(*state->rows));
        if (!state->pixels || !state->rows)
        {
            png_cleanup(state);
            return FVM_OUT_OF_MEMORY;
        }
        for (y = 0; y < h; ++y)
        {
            state->rows[y] = state->pixels + (size_t)y * w * 4;
        }
        png_read_image(state->png, state->rows);
        png_read_end(state->png, state->info);
        if (state->input.position != size)
        {
            png_error(state->png, "Trailing FlashVM PNG data");
        }
        *width = w;
        *height = h;
        *pixels = state->pixels;
        state->pixels = NULL;
        png_cleanup(state);
        return FVM_OK;
    }
#else
    (void)limit;
    return FVM_UNSUPPORTED;
#endif
}


/* Stream one full-canvas GIF89a frame without allowing an animated slurp to allocate many images.
 */
FVM_Result fvm_swf_decode_gif(
    const unsigned char *bytes, size_t size, size_t limit, unsigned *width, unsigned *height, unsigned char **pixels)
{
    if (!width || !height || !pixels)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *width = *height = 0;
    *pixels = NULL;
    if (!bytes || size < 13 || memcmp(bytes, "GIF89a", 6))
    {
        return FVM_INVALID_ARGUMENT;
    }
#ifdef FVM_HAS_IMAGE_CODECS
    {
        FVM_ImageInput input;
        GifFileType *gif;
        GifRecordType record;
        GifByteType *extension;
        ColorMapObject *palette;
        unsigned char *output = NULL, *row = NULL;
        FVM_Result result = FVM_INVALID_ARGUMENT;
        int error = 0, code, transparent = NO_TRANSPARENT_COLOR, seen = 0;
        int x, y, pass, start, step;
        size_t count;
        input.bytes = bytes;
        input.size = size;
        input.position = 0;
        gif = DGifOpen(&input, gif_input, &error);
        if (!gif)
        {
            return gif_result(error);
        }
        if (gif->SWidth <= 0 || gif->SHeight <= 0)
        {
            goto cleanup;
        }
        if (gif->SWidth > 2880 || gif->SHeight > 2880)
        {
            result = FVM_UNSUPPORTED;
            goto cleanup;
        }
        count = (size_t)gif->SWidth * gif->SHeight * 4;
        if (limit && count > limit)
        {
            result = FVM_OUT_OF_MEMORY;
            goto cleanup;
        }
        output = (unsigned char *)malloc(count);
        row = (unsigned char *)malloc((size_t)gif->SWidth);
        if (!output || !row)
        {
            result = FVM_OUT_OF_MEMORY;
            goto cleanup;
        }
        for (;;)
        {
            if (DGifGetRecordType(gif, &record) == GIF_ERROR)
            {
                goto library_error;
            }
            if (record == TERMINATE_RECORD_TYPE)
            {
                break;
            }
            if (record == EXTENSION_RECORD_TYPE)
            {
                if (DGifGetExtension(gif, &code, &extension) == GIF_ERROR)
                {
                    goto library_error;
                }
                if (code == GRAPHICS_EXT_FUNC_CODE)
                {
                    if (!extension || extension[0] != 4)
                    {
                        goto cleanup;
                    }
                    if (!seen)
                    {
                        transparent = extension[1] & 1 ? extension[4] : NO_TRANSPARENT_COLOR;
                    }
                }
                while (extension)
                {
                    if (DGifGetExtensionNext(gif, &extension) == GIF_ERROR)
                    {
                        goto library_error;
                    }
                }
                continue;
            }
            if (record != IMAGE_DESC_RECORD_TYPE)
            {
                goto cleanup;
            }
            if (seen)
            {
                result = FVM_UNSUPPORTED;
                goto cleanup;
            }
            /* Check image dimensions before giflib multiplies them in its native long counter. */
            if (input.size - input.position < 9)
            {
                goto cleanup;
            }
            {
                const unsigned char *header = input.bytes + input.position;
                unsigned image_width = header[4] | ((unsigned)header[5] << 8);
                unsigned image_height = header[6] | ((unsigned)header[7] << 8);
                if (header[0] || header[1] || header[2] || header[3] || image_width != (unsigned)gif->SWidth ||
                    image_height != (unsigned)gif->SHeight)
                {
                    result = FVM_UNSUPPORTED;
                    goto cleanup;
                }
            }
            if (DGifGetImageDesc(gif) == GIF_ERROR)
            {
                goto library_error;
            }
            if (gif->Image.Left || gif->Image.Top || gif->Image.Width != gif->SWidth ||
                gif->Image.Height != gif->SHeight)
            {
                result = FVM_UNSUPPORTED;
                goto cleanup;
            }
            palette = gif->Image.ColorMap ? gif->Image.ColorMap : gif->SColorMap;
            if (!palette || palette->ColorCount <= 0 ||
                (transparent != NO_TRANSPARENT_COLOR && transparent >= palette->ColorCount))
            {
                goto cleanup;
            }
            for (pass = 0; pass < (gif->Image.Interlace ? 4 : 1); ++pass)
            {
                static const int starts[4] = {0, 4, 2, 1};
                static const int steps[4] = {8, 8, 4, 2};
                start = gif->Image.Interlace ? starts[pass] : 0;
                step = gif->Image.Interlace ? steps[pass] : 1;
                for (y = start; y < gif->SHeight; y += step)
                {
                    if (DGifGetLine(gif, row, gif->SWidth) == GIF_ERROR)
                    {
                        goto library_error;
                    }
                    for (x = 0; x < gif->SWidth; ++x)
                    {
                        unsigned char *p = output + ((size_t)y * gif->SWidth + x) * 4;
                        GifColorType *color;
                        if (row[x] >= palette->ColorCount)
                        {
                            goto cleanup;
                        }
                        color = palette->Colors + row[x];
                        p[3] = row[x] == transparent ? 0 : 255;
                        p[0] = p[3] ? color->Red : 0;
                        p[1] = p[3] ? color->Green : 0;
                        p[2] = p[3] ? color->Blue : 0;
                    }
                }
            }
            seen = 1;
        }
        if (!seen || input.position != size)
        {
            goto cleanup;
        }
        *width = (unsigned)gif->SWidth;
        *height = (unsigned)gif->SHeight;
        *pixels = output;
        output = NULL;
        result = FVM_OK;
        goto cleanup;
    library_error:
        result = gif_result(gif->Error);
    cleanup:
        (void)DGifCloseFile(gif, &error);
        free(output);
        free(row);
        return result;
    }
#else
    (void)limit;
    return FVM_UNSUPPORTED;
#endif
}
