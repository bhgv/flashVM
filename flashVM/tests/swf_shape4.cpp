#include <stdio.h>
#include <stdlib.h>
#include "swf_shape4_fixture.hpp"


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
    for (unsigned int kind = 0; kind < 3; ++kind)
    {
        for (unsigned int spread = 0; spread < 3; ++spread)
        {
            for (unsigned int interpolation = 0; interpolation < 2; ++interpolation)
            {
                unsigned int type = kind == 0 ? 0x10 : kind == 1 ? 0x12 : 0x13;
                Bytes source = shape4(type, spread, interpolation, -128);
                FVM_SWFShape decoded;
                if (!(fvm_swf_read_shape(source.begin(), source.size(), 83, decoded) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(decoded.id == 9 && decoded.paints.size() == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(decoded.paints[0].gradient_spread == spread))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(decoded.paints[0].gradient_interpolation == interpolation))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *root = load(c, shape4_tags(source), 1);
                tick(c, root);
                shape4_pixels(c, root, type, spread, interpolation, kind == 2 ? -.5 : 0);
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
