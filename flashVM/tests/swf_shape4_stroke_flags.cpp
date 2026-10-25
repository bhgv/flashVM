#include <stdio.h>
#include <stdlib.h>
#include "swf_shape4_stroke_fixture.hpp"


int main()
{ /* Every 16-bit header: reserved takes precedence, then unsupported semantics. */
    for (unsigned first = 0; first < 256; ++first)
    {
        for (unsigned second = 0; second < 256; ++second)
        {
            Bytes bytes = shape4_stroke(0, 0, 0, 0, 80, 2, first, second);
            FVM_SWFShape out;
            out.id = 777;
            FVM_Result expected = FVM_OK;
            if ((first >> 6) == 3 || ((first >> 4) & 3) == 3 || (second & 3) == 3 || (second >> 3))
            {
                expected = FVM_INVALID_ARGUMENT;
            }
            else if ((first >> 6) != (second & 3) || ((first >> 4) & 3) > 1 || (first & 7))
            {
                expected = FVM_UNSUPPORTED;
            }
            FVM_Result r = fvm_swf_read_shape(bytes.begin(), bytes.size(), 83, out);
            if (!(r == expected))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (r != FVM_OK)
            {
                if (!(out.id == 777 && out.paints.empty()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    return 0;
}
