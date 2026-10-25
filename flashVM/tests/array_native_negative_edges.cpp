/* Crossing zero, raw unsigned input, protected slots and signed-underflow rejection. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"
#include <cstdio>


static void edge_length(FVM_Context *c, FVM_Object *o, double expected)
{
    FVM_Value value;
    if (!(fvm_object_get(c, o, "length", &value) == FVM_OK && value.type == FVM_VALUE_NUMBER &&
          value.number == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
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
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned method = 0; method < 2; ++method)
        {
            for (unsigned count = 0; count <= 4; ++count)
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
                    FVM_Value value, args[4], out;
                    fvm_value_number(&value, -3);
                    FVM_Object *array = 0;
                    if (!(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (fixed)
                    {
                        if (!(fvm_object_set_property_flags(c, array, "length", 4, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    for (unsigned i = 0; i < 4; ++i)
                    {
                        fvm_value_number(args + i, i + 10);
                    }
                    if (!(fvm_object_call(c, array, method ? "unshift" : "push", count, args, &out) == FVM_OK &&
                          out.type == FVM_VALUE_NUMBER && out.number == (double)count - 3))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    edge_length(c, array, fixed ? -3 : (double)count - 3);
                    for (unsigned i = 0; i < count; ++i)
                    {
                        char key[32];
                        std::sprintf(key, "%ld", method ? (long)i : (long)i - 3);
                        int own = 0;
                        if (!(fvm_object_has_own(c, array, key, &own) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        const bool kept = !method || fixed || ((double)count - 3 > i);
                        if (!(!!own == kept))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (kept)
                        {
                            if (!(fvm_object_get(c, array, key, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
                                  out.number == i + 10))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    for (unsigned version = 4; version <= 8; ++version)
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
        FVM_Value value, out, arg;
        fvm_value_number(&value, -3);
        FVM_Object *array = 0;
        if (!(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 81);
        if (!(fvm_array_store_own(c, array, 0, value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_property_flags(c, array, "0", 6, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&arg, 10);
        if (!(fvm_object_call(c, array, "unshift", 1, &arg, &out) == FVM_OK && out.number == -2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        edge_length(c, array, -2);
        if (!(fvm_object_get(c, array, "0", &out) == FVM_OK && out.number == 81))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 4294967295.0);
        if (!(fvm_object_set(c, array, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, array, "push", 1, &arg, &out) == FVM_OK && out.number == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        edge_length(c, array, 0);
        if (!(fvm_object_get(c, array, "-1", &out) == FVM_OK && out.number == 10))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, -2147483648.0);
        FVM_Object *minimum = 0;
        if (!(fvm_construct(c, "Array", 1, &value, &minimum) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 91);
        if (!(fvm_array_store_own(c, minimum, 0, value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, minimum, "pop", 0, 0, &out) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, minimum, "shift", 0, 0, &out) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        edge_length(c, minimum, -2147483648.0);
        if (!(fvm_object_get(c, minimum, "0", &out) == FVM_OK && out.number == 91))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, minimum, "push", 1, &arg, &out) == FVM_OK && out.number == -2147483647.0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        edge_length(c, minimum, -2147483647.0);
        FVM_Object *budget = 0;
        fvm_value_number(&value, -3);
        if (!(fvm_construct(c, "Array", 1, &value, &budget) == FVM_OK && fvm_gc_pin(c, budget) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value many[4];
        for (unsigned i = 0; i < 4; ++i)
        {
            fvm_value_number(many + i, 10 + i);
        }
        const unsigned long old_limit = rt->config.instruction_limit;
        rt->config.instruction_limit = 1;
        if (!(fvm_object_call(c, budget, "unshift", 4, many, &out) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = old_limit;
        if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_call(c, budget, "unshift", 4, many, &out) == FVM_OK))
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
