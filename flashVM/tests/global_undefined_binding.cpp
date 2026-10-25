/* Existing global undefined bindings suppress registered native fallback across mutation. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/property.h"
#include "flashvm/standard.h"

static unsigned global_getter_calls;
static bool global_getter_delete;
static const char *global_key;
static FVM_Object *global_owner;


static FVM_Result global_native(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_number(out, 93);
    return FVM_OK;
}


static FVM_Result
global_getter(FVM_Context *c, FVM_Object *receiver, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(receiver == c->global))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++global_getter_calls;
    if (global_getter_delete)
    {
        int removed = 0;
        if (!(fvm_object_delete(c, global_owner, global_key, &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (unsigned kind = 0; kind < 3; ++kind)
        {
            for (unsigned inherited = 0; inherited < 2; ++inherited)
            {
                if (version && version < 4)
                {
                    continue;
                }
                FVM_Context *c = 0;
                if (!(fvm_context_create(runtime, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (version)
                {
                    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
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
                global_key = "mixedBinding";
                if (!(fvm_register_function(c, global_key, global_native, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                global_owner = c->global;
                if (inherited)
                {
                    if (!(fvm_object_create(c, &global_owner) == FVM_OK &&
                          fvm_object_set_prototype(c, c->global, global_owner) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                FVM_Value value, out;
                fvm_value_undefined(&value);
                global_getter_calls = 0;
                global_getter_delete = kind == 2;
                const char *stored = version && version <= 6 ? "MIXEDBINDING" : global_key;
                if (!kind)
                {
                    if (!(fvm_object_set(c, global_owner, stored, &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(fvm_object_define_accessor_rooted(c, global_owner, stored, global_getter, 0, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_get_global(c, global_key, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED &&
                      global_getter_calls == (kind ? 1U : 0U)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (global_getter_delete)
                {
                    if (!(fvm_get_global(c, global_key, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_function_invoke(c, out.object, c->global, 0, 0, &value) == FVM_OK && value.number == 93))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(!c->gc_frames && fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
