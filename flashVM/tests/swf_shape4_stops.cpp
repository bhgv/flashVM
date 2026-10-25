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
    for (unsigned int count = 1; count <= 15; ++count)
    {
        for (unsigned int mode = 0; mode < 6; ++mode)
        {
            Bytes bytes = shape4(0x10, mode / 2, mode % 2, 0, count);
            FVM_SWFShape decoded;
            if (!(fvm_swf_read_shape(bytes.begin(), bytes.size(), 83, decoded) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(decoded.paints[0].stops.size() == count))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *root = load(c, shape4_tags(bytes), 1);
            tick(c, root);
            shape4_pixels(c, root, 0x10, mode / 2, mode % 2, 0, count);
            if (!(fvm_gc_unpin(c, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    size_t header = 0;
    Bytes duplicate = shape4(0x12, 0, 0, 0, 3, &header);
    duplicate[header + 6] = 0;
    FVM_SWFShape decoded;
    if (!(fvm_swf_read_shape(duplicate.begin(), duplicate.size(), 83, decoded) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(decoded.paints[0].stops[0].ratio == decoded.paints[0].stops[1].ratio))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
