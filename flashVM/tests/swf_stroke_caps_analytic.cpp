#include <stdio.h>
#include <stdlib.h>
#include "swf_stroke_caps_fixture.hpp"


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
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned cap = 0; cap < 3; ++cap)
    {
        FVM_Object *root = load(c, bs_tags(caps_shape(0, cap, 0)), 1);
        tick(c, root);
        unsigned char pixels[1024];
        std::memset(pixels, 0, sizeof(pixels));
        FVM_RasterSurface s = {pixels, 16, 16, 64};
        FVM_Result r = fvm_movieclip_render(c, root, &s);
        if (r == FVM_UNSUPPORTED)
        {
            continue;
        }
        if (!(r == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(pixels[4 * (8 * 16 + 8) + 3] == 128))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (cap == 1)
        {
            for (unsigned y = 0; y < 16; ++y)
            {
                for (unsigned x = 0; x < 16; ++x)
                {
                    if (!(pixels[4 * (y * 16 + x) + 3] == ((x >= 2 && x < 14 && y >= 6 && y < 10) ? 128 : 0)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        if (cap == 2)
        {
            for (unsigned y = 0; y < 16; ++y)
            {
                for (unsigned x = 0; x < 16; ++x)
                {
                    if (!(pixels[4 * (y * 16 + x) + 3] == ((y >= 6 && y < 10) ? 128 : 0)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        if (cap == 0)
        {
            if (!(pixels[4 * (6 * 16) + 3] > 0 && pixels[4 * (6 * 16) + 3] < 128))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    for (unsigned join = 0; join < 2; ++join)
    {
        FVM_Object *root = load(c, bs_tags(caps_shape(0, 0, join, 80, true, false)), 1);
        tick(c, root);
        unsigned char pixels[1024];
        std::memset(pixels, 0, sizeof(pixels));
        FVM_RasterSurface s = {pixels, 16, 16, 64};
        FVM_Result r = fvm_movieclip_render(c, root, &s);
        if (r == FVM_UNSUPPORTED)
        {
            continue;
        }
        if (!(r == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned alpha = pixels[4 * (2 * 16 + 13) + 3];
        if (!(join ? alpha == 0 : alpha > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
