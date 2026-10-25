/* Every C++ allocation boundary preserves caller buffers; retry remains usable. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "textfield_counted_text_allocation.cpp"

#undef main
#include "flashvm/font.h"
#include <cstring>


int main()
{
#ifdef FVM_HAS_FONTS
    bool succeeded = false;
    unsigned failures = 0;
    for (long boundary = 0; boundary < 64 && !succeeded; ++boundary)
    {
        unsigned int text[4] = {0x5d0, 0x5d1, 32, 65};
        unsigned int original[4];
        std::memcpy(original, text, sizeof(text));
        size_t map[4] = {91, 92, 93, 94};
        unsigned char levels[4] = {81, 82, 83, 84};
        FVM_TextDirection direction = FVM_TEXT_AUTO;
        allowance = boundary;
        FVM_Result result = fvm_text_bidi(text, 4, &direction, text, map, levels);
        allowance = -1;
        if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        succeeded = result == FVM_OK;
        if (!succeeded)
        {
            ++failures;
            if (!(direction == FVM_TEXT_AUTO && !std::memcmp(text, original, sizeof(text))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned i = 0; i < 4; ++i)
            {
                if (!(map[i] == 91 + i && levels[i] == 81 + i))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_text_bidi(text, 4, &direction, text, map, levels) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        /* Analytical RTL order: Latin A, space, bet, alef; levels remain logical. */
        if (!(direction == FVM_TEXT_RTL && text[0] == 65 && text[1] == 32 && text[2] == 0x5d1 && text[3] == 0x5d0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(map[0] == 3 && map[1] == 2 && map[2] == 1 && map[3] == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(levels[0] == 1 && levels[1] == 1 && levels[2] == 1 && levels[3] == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(succeeded && failures >= 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
