/* Independent packed-code boundary and transactional-error fixtures. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf_sound.h"
#include "swf_sound_fixture.hpp"

using namespace sound_fixture;


struct Bits
{
    Bytes bytes;
    unsigned n;


    Bits() : n(0)
    {
    }


    void put(unsigned v, unsigned width)
    {
        for (unsigned i = width; i; --i)
        {
            if (!(n % 8))
            {
                bytes.push_back(0);
            }
            bytes[bytes.size() - 1] |= ((v >> (i - 1)) & 1) << (7 - n % 8);
            ++n;
        }
    }
};


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
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
    for (unsigned width = 2; width <= 5; ++width)
    {
        for (unsigned channels = 1; channels <= 2; ++channels)
        {
            Bits b;
            b.put(width - 2, 2);
            for (unsigned frame = 0; frame < 4098; ++frame)
            {
                for (unsigned ch = 0; ch < channels; ++ch)
                {
                    if (frame % 4096 == 0)
                    {
                        b.put(ch ? 65526 : 10, 16);
                        b.put(0, 6);
                    }
                    else
                    {
                        b.put(0, width);
                    }
                }
            }
            Bytes defs;
            tag(defs, 14, sound(1, 0x1e | (channels - 1), 4098, b.bytes.begin(), b.bytes.size()));
            Bytes swf = movie(defs);
            if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            short pcm[8] = {0};
            size_t n = 0;
            if (!(fvm_swf_sound_read_pcm(c, 1, 4095, 3, pcm, 8, &n) == FVM_OK && n == 3 * channels))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(pcm[channels] == 10))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (channels == 2)
            {
                if (!(pcm[channels + 1] == -10))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_swf_sound_read_pcm(c, 1, 0, 1, pcm, 8, &n) == FVM_OK && pcm[0] == 10))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            b.bytes.resize(b.bytes.size() - 1);
            defs.clear();
            tag(defs, 14, sound(1, 0x1e | (channels - 1), 4098, b.bytes.begin(), b.bytes.size()));
            swf = movie(defs);
            if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            pcm[0] = 1234;
            if (!(fvm_swf_sound_read_pcm(c, 1, 0, 1, pcm, 8, &n) == FVM_INVALID_ARGUMENT && !n && pcm[0] == 1234))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_swf_sound_read_pcm(c, 1, 0, 0, 0, 0, &n) == FVM_INVALID_ARGUMENT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
