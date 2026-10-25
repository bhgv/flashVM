/* Native negative splice uses captured signed bounds and native pre-store length growth. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"
#include <cstdio>


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
        for (unsigned count = 0; count <= 4; ++count)
        {
            for (unsigned fixed = 0; fixed < 2; ++fixed)
            {
                for (unsigned raw = 0; raw < 2; ++raw)
                {
                    for (unsigned route = 0; route < 5; ++route)
                    {
                        if (route >= 3 && !fvm_jit_available())
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
                        FVM_Value value, args[6], out;
                        fvm_value_number(&value, -3);
                        FVM_Object *array = 0;
                        if (!(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (raw)
                        {
                            fvm_value_number(&value, 4294967293.0);
                            if (!(fvm_object_set(c, array, "length", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        value = class_value(array);
                        if (!(fvm_set_global(c, "signedSpliceTarget", &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_value_number(&value, 84);
                        if (!(fvm_array_store_own(c, array, 4, value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        version_flags(c, array, "length", fixed ? 0x1007 : 0x1003, 65535);
                        fvm_value_number(args, 0);
                        fvm_value_number(args + 1, 0);
                        for (unsigned i = 0; i < count; ++i)
                        {
                            fvm_value_number(args + i + 2, i + 10);
                        }
                        if (!route)
                        {
                            if (!(fvm_object_call(c, array, "splice", count + 2, args, &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (route == 1)
                        {
                            FVM_Value function;
                            if (!(fvm_object_get(c, array, "splice", &function) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_function_invoke(c, function.object, array, count + 2, args, &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            Program code;
                            for (unsigned i = count; i; --i)
                            {
                                code.number(i + 9);
                            }
                            code.number(0);
                            code.number(0);
                            code.number(count + 2);
                            code.variable("signedSpliceTarget");
                            code.text("splice");
                            code.op(0x52);
                            code.finish();
                            out = boxing_run(c, code, route - 2, FVM_OK);
                        }
                        if (!(out.type == FVM_VALUE_OBJECT))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Value length;
                        if (!(fvm_object_get(c, out.object, "length", &length) == FVM_OK && length.number == 0))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_gc_pin(c, out.object) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        unsigned flags = 0;
                        if (!(fvm_object_property_flags(c, array, "length", &flags) == FVM_OK &&
                              flags == (fixed ? 0x1007 : 3)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        version_flags(c, array, "length", 0, 0xfff8);
                        if (!(fvm_object_get(c, array, "length", &length) == FVM_OK &&
                              length.number == (fixed ? (raw ? 4294967293.0 : -3) : (double)count - 3)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        for (unsigned i = 0; i < count; ++i)
                        {
                            char key[32];
                            std::sprintf(key, "%ld", (long)i - 3);
                            if (!(fvm_object_get(c, array, key, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
                                  out.number == i + 10))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(fvm_object_get(c, array, "4", &out) == FVM_OK && out.number == 84))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth &&
                              !c->calling_owner))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        value = class_value(array);
                        unsigned char *data = 0;
                        size_t bytes = 0;
                        if (!(fvm_value_serialize(c, &value, &data, &bytes) == FVM_OK))
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
                        version_load(other, version);
                        if (!(fvm_value_deserialize(other, data, bytes, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_serialized_free(data);
                        if (!(fvm_object_get(other, out.object, "length", &length) == FVM_OK &&
                              length.number == (fixed ? (raw ? 4294967293.0 : -3) : (double)count - 3)))
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
    }
    fvm_runtime_destroy(rt);
}
