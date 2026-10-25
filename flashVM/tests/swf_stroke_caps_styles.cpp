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
    for (unsigned type = 0; type < 4; ++type)
    {
        for (unsigned cap = 0; cap < 3; ++cap)
        {
            for (unsigned join = 0; join < 2; ++join)
            {
                size_t end = 0, style = 0;
                Bytes b = caps_shape(type, cap, join, 80, false, true, &end),
                      unused = bs_shape(0x42, 80, false, false, 0, 7, &style), bytes;
                (void)unused;
                for (size_t i = 0; i < style; ++i)
                {
                    bytes.push_back(b[i]);
                }
                bytes[bytes.size() - 1] = 0;
                Bits records;
                records.put(0, 4);
                records.put(0, 4);
                records.put(0, 1);
                records.put(16, 5);
                records.bit = 0;
                records.b.push_back(0);
                records.b.push_back(1);
                for (size_t i = style; i < end; ++i)
                {
                    records.b.push_back(b[i]);
                }
                records.put(0, 4);
                records.put(1, 4);
                records.put(0, 1);
                records.put(9, 5);
                records.put(16, 5);
                records.put(40, 16);
                records.put(160, 16);
                records.put(1, 1);
                edge(records, 240, 0);
                records.put(0, 6);
                append(bytes, records.b);
                FVM_Object *root = load(c, bs_tags(bytes), 1);
                tick(c, root);
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                caps_pixels(c, root, type, cap, join, 80, false, true);
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
