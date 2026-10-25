#include <stdio.h>
#include "png.h"
#include "image_codecs_memory.h"
#include <stdlib.h>


int main(void)
{
    png_image image;
    unsigned char source[8 * 8 * 4], decoded[sizeof(source)];
    unsigned char *encoded;
    png_alloc_size_t size = 0;
    unsigned int i;
    if (!(strcmp(png_get_libpng_ver(0), "1.6.59") == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 64; ++i)
    {
        source[i * 4] = (unsigned char)(i * 3);
        source[i * 4 + 1] = 100;
        source[i * 4 + 2] = 200;
        source[i * 4 + 3] = i % 3 == 0 ? 0 : (i % 3 == 1 ? 128 : 255);
    }
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;
    image.width = image.height = 8;
    image.format = PNG_FORMAT_RGBA;
    if (!(png_image_write_to_memory(&image, 0, &size, 0, source, 0, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(size > 8 && size < 65536))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    encoded = (unsigned char *)malloc((size_t)size);
    if (!(encoded))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(png_image_write_to_memory(&image, encoded, &size, 0, source, 0, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(memcmp(encoded, "\211PNG\r\n\032\n", 8) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;
    if (!(png_image_begin_read_from_memory(&image, encoded, (size_t)size)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(image.width == 8 && image.height == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    image.format = PNG_FORMAT_RGBA;
    if (!(png_image_finish_read(&image, 0, decoded, 0, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(memcmp(source, decoded, sizeof(source)) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    png_image_free(&image);
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;
    if (!(!png_image_begin_read_from_memory(&image, "bad", 3)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    png_image_free(&image);
    memset(&image, 0, sizeof(image));
    image.version = PNG_IMAGE_VERSION;
    if (!(!png_image_begin_read_from_memory(&image, encoded, 8)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    png_image_free(&image);
    free(encoded);
    return 0;
}
