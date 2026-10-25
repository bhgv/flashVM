#include <stdio.h>
#include <stdlib.h>
#include "swf_shape4_fixture.hpp"


static Bytes rect(long x0, long x1, long y0, long y1, unsigned bits = 26)
{
    Bits b;
    b.put(bits, 5);
    b.put((unsigned long)x0, bits);
    b.put((unsigned long)x1, bits);
    b.put((unsigned long)y0, bits);
    b.put((unsigned long)y1, bits);
    return b.b;
}


int main()
{
    size_t flags = 0;
    Bytes source = shape4(0x10, 0, 0, 0, 2, 0, &flags);
    FVM_SWFShape out;
    for (unsigned int hints = 0; hints < 4; ++hints)
    {
        Bytes b = source;
        b[flags] = (unsigned char)hints;
        if (!(fvm_swf_read_shape(b.begin(), b.size(), 83, out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    const long values[][4] = {{-20000000, 20000000, -20000000, 20000000},
                              {1, 0, 0, 1},
                              {0, 1, 1, 0},
                              {-20000001, 0, 0, 1},
                              {0, 20000001, 0, 1},
                              {0, 1, -20000001, 0},
                              {0, 1, 0, 20000001}};
    for (unsigned int i = 0; i < 7; ++i)
    {
        Bytes b;
        for (size_t j = 0; j < 2 + (flags - 2) / 2; ++j)
        {
            b.push_back(source[j]);
        }
        append(b, rect(values[i][0], values[i][1], values[i][2], values[i][3]));
        for (size_t j = flags; j < source.size(); ++j)
        {
            b.push_back(source[j]);
        }
        out.id = 777;
        FVM_Result r = fvm_swf_read_shape(b.begin(), b.size(), 83, out);
        if (!(r == (i == 0 ? FVM_OK : FVM_INVALID_ARGUMENT)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (i)
        {
            if (!(out.id == 777))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}
