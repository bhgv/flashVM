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
    Bytes tags, empty, a = shape4(0x13, 1, 1, -128), b = shape4(0x13, 2, 0, 128);
    b[0] = 10;
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
    shape4_pixels(c, root, 0x13, 1, 1, -.5);
    tick(c, root);
    if (!(display_node(root) == node))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    shape4_pixels(c, root, 0x13, 2, 0, .5);
    for (unsigned int i = 0; i < 16; ++i)
    {
        unsigned int frame = i % 2 + 1;
        seek(c, root, frame);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        shape4_pixels(c, root, 0x13, frame == 1 ? 1 : 2, frame == 1 ? 1 : 0, frame == 1 ? -.5 : .5);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
