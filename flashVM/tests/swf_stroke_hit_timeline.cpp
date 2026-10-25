#include <stdio.h>
#include <stdlib.h>
#include "swf_stroke_hit_fixture.hpp"


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
    Bytes tags, empty, a = caps_shape(3, 1, 1, 80, false, false), b = caps_shape(2, 2, 1, 80, false, false);
    b[0] = 10;
    tag(tags, 36, lossless(5, 2, 2, bs_raw()));
    tag(tags, 83, a);
    tag(tags, 83, b);
    tag(tags, 26, place(6, 1, 9, 0, 0));
    tag(tags, 1, empty);
    tag(tags, 26, place(3, 1, 10, 0, 0));
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    FVM_Object *root = load(c, tags, 2);
    tick(c, root);
    FVM_Object *node = display_node(root);
    stroke_hits(c, root, 1);
    tick(c, root);
    if (!(display_node(root) == node))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    stroke_hits(c, root, 2);
    for (unsigned int i = 0; i < 16; ++i)
    {
        unsigned int frame = i % 2 + 1;
        seek(c, root, frame);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        stroke_hits(c, root, frame == 1 ? 1 : 2);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
