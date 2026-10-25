/* A native boxing constructor may return a primitive; ToObject still returns undefined. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/execution.h"

static unsigned primitive_mode, primitive_calls;


static FVM_Result primitive_factory(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    ++primitive_calls;
    if (!(n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    *out = a[0];
    if (primitive_mode == 1)
    {
        fvm_value_undefined(out);
    }
    if (primitive_mode == 2)
    {
        out->type = FVM_VALUE_NULL;
    }
    return FVM_OK;
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
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *factory = 0;
        if (!(fvm_function_create_native(c, primitive_factory, 0, 0, &factory) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        factory->constructable = factory->invocable;
        FVM_Value value = class_value(factory), input[3], out;
        const char *names[] = {"Number", "Boolean", "String"};
        fvm_value_number(input, 42);
        fvm_value_boolean(input + 1, 1);
        if (!(fvm_value_string_bytes(input + 2, "a\0b", 3) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned type = 0; type < 3; ++type)
        {
            if (!(fvm_set_global(c, names[type], &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (primitive_mode = 0; primitive_mode < 3; ++primitive_mode)
            {
                primitive_calls = 0;
                fvm_value_number(&out, 99);
                if (!(fvm_exec_to_object(c, input + type, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED &&
                      primitive_calls == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                out = input[type];
                if (!(fvm_exec_to_object(c, &out, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED &&
                      primitive_calls == 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
