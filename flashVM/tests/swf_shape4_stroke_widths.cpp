#include <stdio.h>
#include <stdlib.h>
#include "swf_shape4_stroke_fixture.hpp"


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
    const unsigned widths[] = {0, 1, 20, 80, 65535};
    for (unsigned k = 0; k < 3; ++k)
    {
        for (unsigned i = 0; i < 5; ++i)
        {
            unsigned kind = k == 0 ? 0 : k == 1 ? 0x10 : 0x13;
            Bytes bytes = shape4_stroke(kind, 2, 1, 128, widths[i]);
            FVM_SWFShape out;
            if (!(fvm_swf_read_shape(bytes.begin(), bytes.size(), 83, out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(out.paints[0].width == widths[i] / 20.0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *root = load(c, shape4_tags(bytes), 1);
            tick(c, root);
            stroke4_pixels(c, root, kind, 2, 1, kind == 0x13 ? .5 : 0, widths[i]);
        }
    }
    /* Direct color and solid FillType are equivalent, including alpha. */
    FVM_Object *root = load(c, shape4_tags(shape4_stroke(0, 0, 0, 0, 80, 2, 0)), 1);
    tick(c, root);
    stroke4_pixels(c, root, 0, 0, 0, 0);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
