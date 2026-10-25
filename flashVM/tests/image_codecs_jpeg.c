#include <stddef.h>
#include <stdio.h>
#include "jpeglib.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>


typedef struct TestJpegError
{
    struct jpeg_error_mgr base;
    jmp_buf jump;
} TestJpegError;


static void jpeg_failure(j_common_ptr info)
{
    TestJpegError *error = (TestJpegError *)info->err;
    longjmp(error->jump, 1);
}


int main(void)
{
    struct jpeg_compress_struct encoder;
    struct jpeg_error_mgr error;
    struct jpeg_decompress_struct *decoder;
    TestJpegError *decode_error;
    unsigned char *encoded = 0;
    unsigned long size = 0;
    unsigned char row[8 * 3];
    JSAMPROW rows[1];
    unsigned int i;
    memset(&encoder, 0, sizeof(encoder));
    encoder.err = jpeg_std_error(&error);
    jpeg_create_compress(&encoder);
    jpeg_mem_dest(&encoder, &encoded, &size);
    encoder.image_width = encoder.image_height = 8;
    encoder.input_components = 3;
    encoder.in_color_space = JCS_RGB;
    jpeg_set_defaults(&encoder);
    jpeg_set_quality(&encoder, 95, TRUE);
    jpeg_start_compress(&encoder, TRUE);
    for (i = 0; i < 8; ++i)
    {
        row[i * 3] = 200;
        row[i * 3 + 1] = 100;
        row[i * 3 + 2] = 50;
    }
    rows[0] = row;
    while (encoder.next_scanline < encoder.image_height)
    {
        if (!(jpeg_write_scanlines(&encoder, rows, 1) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    jpeg_finish_compress(&encoder);
    jpeg_destroy_compress(&encoder);
    if (!(size > 20 && encoded[0] == 255 && encoded[1] == 216))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Heap state remains defined when libjpeg returns through longjmp. */
    decoder = (struct jpeg_decompress_struct *)calloc(1, sizeof(*decoder));
    decode_error = (TestJpegError *)calloc(1, sizeof(*decode_error));
    if (!(decoder && decode_error))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    decoder->err = jpeg_std_error(&decode_error->base);
    decode_error->base.error_exit = jpeg_failure;
    if (setjmp(decode_error->jump) != 0)
    {
        abort();
    }
    jpeg_create_decompress(decoder);
    jpeg_mem_src(decoder, encoded, size);
    if (!(jpeg_read_header(decoder, TRUE) == JPEG_HEADER_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    decoder->out_color_space = JCS_RGB;
    if (!(jpeg_start_decompress(decoder)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(decoder->output_width == 8 && decoder->output_height == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    while (decoder->output_scanline < decoder->output_height)
    {
        if (!(jpeg_read_scanlines(decoder, rows, 1) == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < 8; ++i)
        {
            if (!(abs((int)row[i * 3] - 200) <= 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(abs((int)row[i * 3 + 1] - 100) <= 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(abs((int)row[i * 3 + 2] - 50) <= 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(jpeg_finish_decompress(decoder)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    jpeg_destroy_decompress(decoder);
    memset(decoder, 0, sizeof(*decoder));
    decoder->err = &decode_error->base;
    if (setjmp(decode_error->jump) == 0)
    {
        jpeg_create_decompress(decoder);
        jpeg_mem_src(decoder, (const unsigned char *)"not a jpeg", 10);
        jpeg_read_header(decoder, TRUE);
        if (!(!"Invalid JPEG must call the supplied error handler"))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    jpeg_destroy_decompress(decoder);
    free(decoder);
    free(decode_error);
    free(encoded);
    return 0;
}
