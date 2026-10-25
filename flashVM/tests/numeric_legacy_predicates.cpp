#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"

static unsigned primitive_calls, text_calls;


static FVM_Result primitive(FVM_Context *, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++primitive_calls;
    *out = object(self);
    return FVM_OK;
}


static FVM_Result text(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++text_calls;
    return fvm_value_string(out, "23");
}


static FVM_Result failure(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    return FVM_ERROR;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value arg, out, value;
        FVM_Object *plain = 0, *fn = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_native(c, primitive, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = object(fn);
        if (!(fvm_object_set(c, plain, "valueOf", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_native(c, text, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = object(fn);
        if (!(fvm_object_set(c, plain, "toString", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const char *names[] = {"isNaN", "isFinite"};
        for (unsigned i = 0; i < 2; ++i)
        {
            if (!(fvm_call(c, names[i], 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN &&
                  !!out.boolean == (i == 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&arg);
            if (!(fvm_call(c, names[i], 1, &arg, &out) == FVM_OK &&
                  !!out.boolean == ((version && version < 7) ? i != 0 : i == 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            arg = object(plain);
            primitive_calls = text_calls = 0;
            if (!(fvm_call(c, names[i], 1, &arg, &out) == FVM_OK && !!out.boolean == (version >= 5 ? i == 0 : i != 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(primitive_calls == 1 && text_calls == (version ? 0U : 1U)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_function_create_native(c, failure, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = object(fn);
        if (!(fvm_object_set(c, plain, "valueOf", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        arg = object(plain);
        if (!(fvm_call(c, "isNaN", 1, &arg, &out) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_call(c, "isFinite", 1, &arg, &out) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->gc_frames == 0 && c->primitive_depth == 0 && c->string_depth == 0 && c->avm1_execution_depth == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
