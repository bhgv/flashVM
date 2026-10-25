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
    Bytes bytes = movie(shape4_tags(shape4()), 1);
    for (unsigned int v = 1; v <= 8; ++v)
    {
        bytes[3] = (unsigned char)v;
        FVM_Object *root = 0;
        FVM_Result r = fvm_movieclip_load_swf(c, bytes.begin(), bytes.size(), "root", &root);
        if (!(r == (v < 8 ? FVM_INVALID_ARGUMENT : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(v == 8 || !root))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Bytes tags, empty;
    tag(tags, 83, shape4());
    tag(tags, 83, shape4());
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    bytes = movie(tags, 1);
    FVM_Object *root = 0;
    if (!(fvm_movieclip_load_swf(c, bytes.begin(), bytes.size(), "root", &root) == FVM_INVALID_ARGUMENT && !root))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Character definitions are forbidden inside DefineSprite timelines. */
    Bytes nested;
    u16(nested, 7);
    u16(nested, 1);
    tag(nested, 83, shape4());
    tag(nested, 1, empty);
    tag(nested, 0, empty);
    tags.clear();
    tag(tags, 39, nested);
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    bytes = movie(tags, 1);
    if (!(fvm_movieclip_load_swf(c, bytes.begin(), bytes.size(), "root", &root) == FVM_INVALID_ARGUMENT && !root))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
