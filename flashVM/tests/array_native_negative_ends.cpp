/* Native signed end operations retain raw values, grow before stores and truncate after unshift. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"


static void signed_number(FVM_Context *c, FVM_Object *o, const char *name, double want)
{
    FVM_Value out;
    if (!(fvm_object_get(c, o, name, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == want))
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
    const char *methods[] = {"push", "unshift", "pop", "shift"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned method = 0; method < 4; ++method)
        {
            for (unsigned fixed = 0; fixed < 2; ++fixed)
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
                    FVM_Value value, args[2], out;
                    fvm_value_number(&value, -3);
                    FVM_Object *array = 0;
                    if (!(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_number(&value, 71);
                    if (!(fvm_base_store_data(c, array, "-3", value, false) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_number(&value, 72);
                    if (!(fvm_base_store_data(c, array, "-4", value, false) == FVM_OK))
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
                    fvm_value_number(&value, 84);
                    if (!(fvm_array_store_own(c, array, 4, value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = class_value(array);
                    if (!(fvm_set_global(c, "signedEndTarget", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_flags(c, array, "length", fixed ? 0x1007 : 0x1003, 65535);
                    fvm_value_number(args, 10);
                    fvm_value_number(args + 1, 11);
                    const unsigned argc = method < 2 ? 2 : 0;
                    if (!route)
                    {
                        if (!(fvm_object_call(c, array, methods[method], argc, args, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (route == 1)
                    {
                        FVM_Value function;
                        if (!(fvm_object_get(c, array, methods[method], &function) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_function_invoke(c, function.object, array, argc, args, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        Program code;
                        if (argc)
                        {
                            code.number(11);
                            code.number(10);
                        }
                        code.number(argc);
                        code.variable("signedEndTarget");
                        code.text(methods[method]);
                        code.op(0x52);
                        code.finish();
                        out = boxing_run(c, code, route - 2, FVM_OK);
                    }
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == (method < 2 ? -1 : method == 2 ? 72 : 81)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    /* Reveal version-hidden length without disturbing its readonly bit. */
                    unsigned flags = 0;
                    if (!(fvm_object_property_flags(c, array, "length", &flags) == FVM_OK &&
                          flags == (fixed ? 0x1007 : 3)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_flags(c, array, "length", 0, 0xfff8);
                    signed_number(c, array, "length", fixed ? -3 : method < 2 ? -1 : -4);
                    signed_number(c, array, "4", 84);
                    if (!method)
                    {
                        signed_number(c, array, "-3", 10);
                        signed_number(c, array, "-2", 11);
                    }
                    if (method == 1)
                    {
                        int own = 0;
                        if (!(fvm_object_has_own(c, array, "0", &own) == FVM_OK && !!own == !!fixed))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_has_own(c, array, "1", &own) == FVM_OK && !!own == !!fixed))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (fixed)
                        {
                            signed_number(c, array, "0", 10);
                            signed_number(c, array, "1", 11);
                        }
                    }
                    if (method == 2)
                    {
                        int own = 1;
                        if (!(fvm_object_has_own(c, array, "-4", &own) == FVM_OK && !own))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->calling_owner))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
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
                    signed_number(other, out.object, "length", fixed ? -3 : method < 2 ? -1 : -4);
                    fvm_context_destroy(other);
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
