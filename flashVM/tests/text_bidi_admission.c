/* C89 hostile enum/UTF-32 admission and failure-atomic output contracts. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/font.h"
#include <string.h>


static void rejected(unsigned int *input, size_t count, FVM_TextDirection *direction)
{
    unsigned int visual[3] = {91, 92, 93};
    size_t map[3] = {81, 82, 83};
    unsigned char levels[3] = {71, 72, 73};
    unsigned char before[sizeof(FVM_TextDirection)];
    memcpy(before, direction, sizeof(before));
    if (!(fvm_text_bidi(input, count, direction, visual, map, levels) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(visual[0] == 91 && visual[1] == 92 && visual[2] == 93))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(map[0] == 81 && map[1] == 82 && map[2] == 83))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(levels[0] == 71 && levels[1] == 72 && levels[2] == 73))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!memcmp(before, direction, sizeof(before))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    unsigned int input[3] = {65, 0x5d0, 0x5d1}, visual[3];
    size_t map[3];
    unsigned char levels[3];
    FVM_TextDirection direction = FVM_TEXT_AUTO;
    rejected(input, 1000001, &direction);
    input[1] = 0xd800;
    rejected(input, 3, &direction);
    input[1] = 0xdfff;
    rejected(input, 3, &direction);
    input[1] = 0x110000;
    rejected(input, 3, &direction);
    input[1] = 0x5d0;
    memset(&direction, 255, sizeof(direction));
    rejected(input, 3, &direction);
    rejected(0, 0, &direction);
    direction = FVM_TEXT_AUTO;
    if (!(fvm_text_bidi(input, 3, 0, visual, map, levels) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_bidi(0, 3, &direction, visual, map, levels) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_bidi(input, 3, &direction, 0, map, levels) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_bidi(input, 3, &direction, visual, 0, levels) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_text_bidi(input, 3, &direction, visual, map, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef FVM_HAS_FONTS
    if (!(fvm_text_bidi(0, 0, &direction, 0, 0, 0) == FVM_OK && direction == FVM_TEXT_LTR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    direction = FVM_TEXT_RTL;
    if (!(fvm_text_bidi(0, 0, &direction, 0, 0, 0) == FVM_OK && direction == FVM_TEXT_RTL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    direction = FVM_TEXT_AUTO;
    if (!(fvm_text_bidi(input, 3, &direction, visual, map, levels) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(direction == FVM_TEXT_LTR && visual[1] == 0x5d1 && map[1] == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#else
    if (!(fvm_text_bidi(input, 3, &direction, visual, map, levels) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(direction == FVM_TEXT_AUTO))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
