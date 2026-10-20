/* FlashVM: strict JPEG resource decoding isolated from C++ exception and destructor scopes. */
#include "swf_jpeg_internal.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#ifdef FVM_HAS_IMAGE_CODECS
#include <stdio.h>
#include <setjmp.h>
#include "jpeglib.h"
#include "jerror.h"

/* Heap state remains valid after setjmp/longjmp; decoder allocations have one cleanup path. */
typedef struct FVM_JpegDecode
{
    struct jpeg_decompress_struct decoder; /* libjpeg decoder and its allocation pools. */
    struct jpeg_error_mgr error;           /* Error callbacks referencing this heap state. */
    jmp_buf jump;                          /* Return point entirely inside the C bridge. */
    FVM_Result result;                     /* Error code retained across longjmp. */
    unsigned char *pixels;                 /* Owned output allocation until publication. */
    unsigned char *row;                    /* Temporary RGB scanline allocation. */
} FVM_JpegDecode;

/* Return fatal libjpeg errors through the C boundary, preserving an allocation-specific code. */
static void jpeg_error(j_common_ptr info)
{
    FVM_JpegDecode *state = (FVM_JpegDecode *)info->client_data;
    state->result =
        info->err->msg_code == JERR_OUT_OF_MEMORY ? FVM_OUT_OF_MEMORY : FVM_INVALID_ARGUMENT;
    longjmp(state->jump, 1);
}

/* Reject warning-based recovery, especially truncated streams repaired with a synthetic EOI. */
static void jpeg_message(j_common_ptr info, int level)
{
    if (level < 0)
        jpeg_error(info);
}

/* Release native decoder pools and all unpublished allocations after success or failure. */
static void jpeg_cleanup(FVM_JpegDecode *state)
{
    jpeg_destroy_decompress(&state->decoder);
    free(state->row);
    free(state->pixels);
    free(state);
}
#endif

/* Bound decoded dimensions before starting decompression and publish only a complete RGB image. */
FVM_Result fvm_swf_decode_jpeg_versioned(const unsigned char *bytes, size_t size, size_t limit,
                                         int allow_progressive, unsigned *width, unsigned *height,
                                         unsigned char **pixels)
{
    if (!width || !height || !pixels)
        return FVM_INVALID_ARGUMENT;
    *width = *height = 0;
    *pixels = NULL;
    if (!bytes || size < 4 || size > ULONG_MAX || bytes[0] != 255 || bytes[1] != 216)
        return FVM_INVALID_ARGUMENT;
#ifdef FVM_HAS_IMAGE_CODECS
    {
        FVM_JpegDecode *state;
        FVM_Result result;
        size_t pixel_bytes;
        unsigned x, y;
        JSAMPROW rows[1];
        state = (FVM_JpegDecode *)calloc(1, sizeof(*state));
        if (!state)
            return FVM_OUT_OF_MEMORY;
        state->result = FVM_INVALID_ARGUMENT;
        state->decoder.err = jpeg_std_error(&state->error);
        state->error.error_exit = jpeg_error;
        state->error.emit_message = jpeg_message;
        state->decoder.client_data = state;
        if (setjmp(state->jump))
        {
            result = state->result;
            jpeg_cleanup(state);
            return result;
        }
        jpeg_create_decompress(&state->decoder);
        jpeg_mem_src(&state->decoder, bytes, (unsigned long)size);
        if (jpeg_read_header(&state->decoder, TRUE) != JPEG_HEADER_OK)
        {
            jpeg_cleanup(state);
            return FVM_INVALID_ARGUMENT;
        }
        if (state->decoder.progressive_mode && !allow_progressive)
        {
            jpeg_cleanup(state);
            return FVM_UNSUPPORTED;
        }
        if (!state->decoder.image_width || !state->decoder.image_height)
        {
            jpeg_cleanup(state);
            return FVM_INVALID_ARGUMENT;
        }
        if (state->decoder.image_width > 2880 || state->decoder.image_height > 2880 ||
            (state->decoder.jpeg_color_space != JCS_GRAYSCALE &&
             state->decoder.jpeg_color_space != JCS_RGB &&
             state->decoder.jpeg_color_space != JCS_YCbCr))
        {
            jpeg_cleanup(state);
            return FVM_UNSUPPORTED;
        }
        pixel_bytes = (size_t)state->decoder.image_width * state->decoder.image_height * 4;
        if (limit && pixel_bytes > limit)
        {
            jpeg_cleanup(state);
            return FVM_OUT_OF_MEMORY;
        }
        state->pixels = (unsigned char *)malloc(pixel_bytes);
        state->row = (unsigned char *)malloc((size_t)state->decoder.image_width * 3);
        if (!state->pixels || !state->row)
        {
            jpeg_cleanup(state);
            return FVM_OUT_OF_MEMORY;
        }
        state->decoder.out_color_space = JCS_RGB;
        if (!jpeg_start_decompress(&state->decoder) || state->decoder.output_components != 3)
        {
            jpeg_cleanup(state);
            return FVM_INVALID_ARGUMENT;
        }
        rows[0] = state->row;
        for (y = 0; y < state->decoder.output_height; ++y)
        {
            if (jpeg_read_scanlines(&state->decoder, rows, 1) != 1)
            {
                jpeg_cleanup(state);
                return FVM_INVALID_ARGUMENT;
            }
            for (x = 0; x < state->decoder.output_width; ++x)
            {
                unsigned char *dest =
                    state->pixels + ((size_t)y * state->decoder.output_width + x) * 4;
                memcpy(dest, state->row + x * 3, 3);
                dest[3] = 255;
            }
        }
        if (!jpeg_finish_decompress(&state->decoder) || state->decoder.src->bytes_in_buffer)
        {
            jpeg_cleanup(state);
            return FVM_INVALID_ARGUMENT;
        }
        *width = state->decoder.output_width;
        *height = state->decoder.output_height;
        *pixels = state->pixels;
        state->pixels = NULL;
        jpeg_cleanup(state);
        return FVM_OK;
    }
#else
    (void)limit;
    (void)allow_progressive;
    return FVM_UNSUPPORTED;
#endif
}

/* Embedded callers retain their existing progressive-capable decoding contract. */
FVM_Result fvm_swf_decode_jpeg(const unsigned char *bytes, size_t size, size_t limit,
                               unsigned *width, unsigned *height, unsigned char **pixels)
{
    return fvm_swf_decode_jpeg_versioned(bytes, size, limit, 1, width, height, pixels);
}
