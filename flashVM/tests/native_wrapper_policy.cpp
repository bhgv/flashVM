/* Trusted internal entry policy changes invalidate wrappers while saved Functions keep their policy. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/standard.h"

static bool policy_trusted;


static FVM_Result policy_native(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    if (!((c->gc_native_depth == 0) == policy_trusted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (policy_trusted)
    {
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_number(out, 42);
    return FVM_OK;
}


static FVM_Result policy_read(FVM_Context *c, FVM_Object *owner, unsigned global, FVM_Value *out)
{
    return global ? fvm_get_global(c, "policy", out) : fvm_object_get(c, owner, "policy", out);
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
        for (unsigned global = 0; global < 2; ++global)
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
            FVM_Object *owner = c->global;
            if (global)
            {
                if (!(fvm_register_function(c, "policy", policy_native, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(fvm_object_create(c, &owner) == FVM_OK && fvm_gc_pin(c, owner) == FVM_OK &&
                      fvm_object_define_method_rooted(c, owner, "policy", policy_native, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            FVM_Value original, trusted, restored, out;
            if (!(policy_read(c, owner, global, &original) == FVM_OK && fvm_gc_pin(c, original.object) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            policy_trusted = false;
            if (!(fvm_function_invoke(c, original.object, owner, 0, 0, &out) == FVM_OK && out.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_NativeEntry &entry =
                global ? c->functions.find("policy")->second : owner->methods.find("policy")->second;
            entry.collector_entry = true; /* Internal fixture only: public callbacks remain guarded. */
            if (!(policy_read(c, owner, global, &trusted) == FVM_OK && trusted.object != original.object &&
                  trusted.object->invocable.collector_entry && fvm_gc_pin(c, trusted.object) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            policy_trusted = true;
            if (!(fvm_function_invoke(c, trusted.object, owner, 0, 0, &out) == FVM_OK && out.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            entry.collector_entry = false;
            if (!(policy_read(c, owner, global, &restored) == FVM_OK && restored.object != trusted.object &&
                  !restored.object->invocable.collector_entry))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            policy_trusted = false;
            if (!(fvm_function_invoke(c, restored.object, owner, 0, 0, &out) == FVM_OK &&
                  fvm_function_invoke(c, original.object, owner, 0, 0, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            policy_trusted = true;
            if (!(fvm_function_invoke(c, trusted.object, owner, 0, 0, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!c->gc_frames && !c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
