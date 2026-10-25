/* Loaded Math methods/constants are readonly/nonenumerable/nondeletable; host defaults remain. */
#include <stdio.h>
#include <stdlib.h>
#include "math_attribute_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (unsigned before = 0; before < 2; ++before)
        {
            for (unsigned field = 0; field < sizeof(math_fields) / sizeof(math_fields[0]); ++field)
            {
                for (unsigned route = 0; route < 4; ++route)
                {
                    if ((version && version < 4) || (!version && before) || (route >= 2 && !fvm_jit_available()))
                    {
                        continue;
                    }
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                    swf[3] = (unsigned char)version;
                    if (before)
                    {
                        if (!(fvm_standard_install(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (version)
                    {
                        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!before)
                    {
                        if (!(fvm_standard_install(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    FVM_Value math, original, out, value;
                    if (!(fvm_get_global(c, "Math", &math) == FVM_OK &&
                          fvm_object_get(c, math.object, math_fields[field], &original) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_set_global(c, "mathNamespace", &math) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    unsigned expected = version ? 7U : field < 8 ? 0U : 1U;
                    math_flag(c, math.object, math_fields[field], expected);
                    math_enumerable(c, math.object, math_fields[field], !(expected & 1U));
                    fvm_value_number(&value, 81);
                    if (!route)
                    {
                        if (!(fvm_object_set(c, math.object, math_fields[field], &value) == FVM_OK &&
                              fvm_object_get(c, math.object, math_fields[field], &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        class_mode = route - 1;
                        Program code;
                        code.variable("mathNamespace");
                        code.text(math_fields[field]);
                        code.number(81);
                        code.op(0x4f);
                        code.variable("mathNamespace");
                        code.text(math_fields[field]);
                        code.op(0x4e);
                        code.finish();
                        out = class_run(c, code);
                    }
                    if (version)
                    {
                        math_same(original, out);
                    }
                    else
                    {
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == 81))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    int removed = 0;
                    if (!(fvm_object_delete(c, math.object, math_fields[field], &removed) == FVM_OK &&
                          !!removed == !version))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (version && !route)
                    {
                        FVM_Context *other = 0;
                        if (!(fvm_context_create(rt, &other) == FVM_OK &&
                              fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        unsigned char *data = 0;
                        size_t bytes = 0;
                        if (!(fvm_value_serialize(c, &math, &data, &bytes) == FVM_OK &&
                              fvm_value_deserialize(other, data, bytes, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_serialized_free(data);
                        FVM_Object *copy = out.object;
                        if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        math_flag(other, copy, math_fields[field], 7);
                        math_enumerable(other, copy, math_fields[field], false);
                        if (!(fvm_object_set(other, copy, math_fields[field], &value) == FVM_OK &&
                              fvm_object_get(other, copy, math_fields[field], &out) == FVM_OK &&
                              out.type == original.type))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (out.type == FVM_VALUE_NUMBER)
                        {
                            if (!(out.number == original.number))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(fvm_object_delete(other, copy, math_fields[field], &removed) == FVM_OK && !removed))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_context_destroy(other);
                    }
                    if (!(!c->gc_frames && !c->gc_native_depth))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
