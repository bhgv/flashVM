#include <stdio.h>
#include <stdlib.h>
#include "../src/capi/swf_jpeg_internal.h"
#include "../src/capi/swf_image_internal.h"


int main()
{
    const unsigned char jpeg[] = {255, 216, 255, 217};
    unsigned width = 99, height = 99;
    unsigned char *pixels = (unsigned char *)1;
    if (!(fvm_swf_decode_jpeg(0, 0, 1024, &width, &height, &pixels) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!width && !height && !pixels))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifndef FVM_HAS_IMAGE_CODECS
    const unsigned char png[33] = {137, 80, 78, 71, 13, 10, 26, 10};
    const unsigned char gif[13] = {71, 73, 70, 56, 57, 97};
    if (!(fvm_swf_decode_png(png, sizeof(png), 1024, &width, &height, &pixels) == FVM_UNSUPPORTED && !pixels))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_decode_gif(gif, sizeof(gif), 1024, &width, &height, &pixels) == FVM_UNSUPPORTED && !pixels))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_decode_jpeg(jpeg, sizeof(jpeg), 1024, &width, &height, &pixels) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!width && !height && !pixels))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    (void)jpeg;
#endif
    return 0;
}
