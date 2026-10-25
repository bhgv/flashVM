/* Number assignment preserves raw length while Int32 controls truncation, flags and graph transfer. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"
#include <limits>


static void numeric_length_equal(const FVM_Value &actual, const FVM_Value &expected)
{
    if (!(actual.type == FVM_VALUE_NUMBER))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (expected.number != expected.number)
    {
        if (!(actual.number != actual.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(actual.number == expected.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!expected.number)
        {
            if (!(!std::memcmp(&actual.number, &expected.number, sizeof(double))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const double values[] = {-3,
                             -3.9,
                             -0.0,
                             0.9,
                             1.9,
                             2.9,
                             4294967295.0,
                             4294967297.9,
                             -4294967296.0,
                             -4294967295.0,
                             -2147483649.0,
                             std::numeric_limits<double>::max(),
                             std::numeric_limits<double>::infinity(),
                             -std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::quiet_NaN()};
    const long bounds[] = {-3, -3, 0, 0, 1, 2, -1, 1, 0, 1, 2147483647L, 0, 0, 0, 0};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
        {
            for (unsigned fixed = 0; fixed < 2; ++fixed)
            {
                for (unsigned route = 0; route < 4; ++route)
                {
                    if (route >= 2 && !fvm_jit_available())
                    {
                        continue;
                    }
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
                    FVM_Value args[3], input, out;
                    for (unsigned j = 0; j < 3; ++j)
                    {
                        fvm_value_number(args + j, 10 + j);
                    }
                    FVM_Object *array = 0, *meta = 0;
                    if (!(fvm_construct(c, "Array", 3, args, &array) == FVM_OK &&
                          fvm_object_create(c, &meta) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value target = class_value(array);
                    if (!(fvm_set_global(c, "numericLengthTarget", &target) == FVM_OK &&
                          fvm_object_set(c, meta, "cycle", &target) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_set_property_flags(c, array, "2", 2, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_flags(c, array, "length", fixed ? 0x1007 : 0x1003, 65535);
                    fvm_value_number(&input, values[i]);
                    input.metatable = meta;
                    if (!route)
                    {
                        if (!(fvm_object_set(c, array, "length", &input) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(fvm_set_global(c, "numericLengthInput", &input) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        Program code;
                        code.variable("numericLengthTarget");
                        code.text("length");
                        code.variable("numericLengthInput");
                        code.op(0x4f);
                        code.number(1);
                        code.finish();
                        out = boxing_run(c, code, route - 1, FVM_OK);
                    }
                    unsigned flags = 0;
                    if (!(fvm_object_property_flags(c, array, "length", &flags) == FVM_OK &&
                          flags == (fixed ? 0x1007 : 3)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_flags(c, array, "length", 0, 0xfff8);
                    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, array, "length", &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value expected;
                    fvm_value_number(&expected, fixed ? 3 : values[i]);
                    numeric_length_equal(out, expected);
                    if (!(out.metatable == (fixed ? 0 : meta)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    for (unsigned j = 0; j < 3; ++j)
                    {
                        char key[2] = {(char)('0' + j), 0};
                        int own = 0;
                        if (!(fvm_object_has_own(c, array, key, &own) == FVM_OK &&
                              !!own == (j == 2 || (long)j < bounds[i])))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    unsigned char *data = 0;
                    size_t bytes = 0;
                    if (!(fvm_value_serialize(c, &target, &data, &bytes) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Context *other = 0;
                    if (!(fvm_context_create(rt, &other) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_value_deserialize(other, data, bytes, &out) == FVM_OK))
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
                    if (!(fvm_object_get(other, copy, "length", &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    numeric_length_equal(out, expected);
                    if (!fixed)
                    {
                        if (!(out.metatable && fvm_object_get(other, out.metatable, "cycle", &out) == FVM_OK &&
                              out.object == copy))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(other);
                    fvm_context_destroy(c);
                }
            }
        }
    }
    /* Unloaded embedding keeps its existing Uint32/integer setter policy. */
    for (unsigned i = 0; i < sizeof(values) / sizeof(values[0]); ++i)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value initial[3], input, out;
        for (unsigned j = 0; j < 3; ++j)
        {
            fvm_value_number(initial + j, j + 10);
        }
        FVM_Object *array = 0;
        if (!(fvm_construct(c, "Array", 3, initial, &array) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&input, values[i]);
        const bool valid = values[i] >= 0 && values[i] <= 4294967295.0 && values[i] == std::floor(values[i]);
        if (!(fvm_object_set(c, array, "length", &input) == (valid ? FVM_OK : FVM_INVALID_ARGUMENT)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, array, "length", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
              out.number == (valid ? values[i] : 3) && !out.metatable))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->gc_frames && !c->gc_native_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
