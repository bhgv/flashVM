/* Numeric raw length is preserved by readers and normalized only when a method writes length. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"
#include <limits>


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const double raw[] = {-3.9,
                          -0.0,
                          0.9,
                          1.9,
                          2.9,
                          4294967297.9,
                          4294967295.0,
                          std::numeric_limits<double>::infinity(),
                          std::numeric_limits<double>::quiet_NaN(),
                          std::numeric_limits<double>::max()};
    const long bounds[] = {-3, 0, 0, 1, 2, 1, -1, 0, 0, 0};
    const char *methods[] = {"join", "sort", "reverse", "slice", "concat", "push", "pop", "shift", "unshift", "splice"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned input = 0; input < sizeof(raw) / sizeof(raw[0]); ++input)
        {
            for (unsigned method = 0; method < 10; ++method)
            {
                for (unsigned fixed = 0; fixed < 2; ++fixed)
                {
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_load(c, version);
                    if (!(fvm_standard_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *array = 0, *meta = 0;
                    if (!(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value value = class_value(array), out, argument;
                    if (!(fvm_set_global(c, "rawLengthRoot", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_number(&value, raw[input]);
                    value.metatable = meta;
                    if (!(fvm_object_set(c, array, "length", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    for (unsigned i = 0; i < 3; ++i)
                    {
                        fvm_value_number(&value, (i + 1) * 11);
                        if (!(fvm_array_store_own(c, array, i, value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (fixed)
                    {
                        if (!(fvm_object_set_property_flags(c, array, "length", 4, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    fvm_value_number(&argument, method == 1 ? 16 : 0);
                    if (!(fvm_object_call(
                              c, array, methods[method], method == 1 || method == 9 ? 1 : 0, &argument, &out) ==
                          FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!method)
                    {
                        const char *want = bounds[input] > 1 ? "11,22" : bounds[input] == 1 ? "11" : "";
                        if (!(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, want)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (method == 1 || method == 2)
                    {
                        if (!(out.type == FVM_VALUE_OBJECT && out.object == array))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (method == 3 || method == 4 || method == 9)
                    {
                        if (!(out.type == FVM_VALUE_OBJECT))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_get(c, out.object, "length", &value) == FVM_OK &&
                              value.number == (bounds[input] > 0 ? bounds[input] : 0)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    const bool writes = !fixed && (method == 5 || method == 8 || method == 9 ||
                                                   ((method == 6 || method == 7) && bounds[input]));
                    double expected = raw[input];
                    if (writes)
                    {
                        expected = method == 9 && bounds[input] > 0 ? 0
                                   : (method == 6 || method == 7)   ? bounds[input] - 1
                                                                    : bounds[input];
                    }
                    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, array, "length", &value) == FVM_OK &&
                          value.type == FVM_VALUE_NUMBER))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (expected != expected)
                    {
                        if (!(value.number != value.number))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(value.number == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!expected && !writes)
                    {
                        if (!(!std::memcmp(&value.number, raw + input, sizeof(double))))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(value.metatable == (writes ? 0 : meta)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_owner))
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
