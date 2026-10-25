/* Native Function factories publish only valid objects from their owning context. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "flashvm/standard.h"

static unsigned factory_result_mode;
static FVM_Object *factory_local, *factory_foreign;


static FVM_Result factory_result(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    if (!factory_result_mode)
    {
        return FVM_OK;
    }
    if (factory_result_mode == 1)
    {
        fvm_value_number(out, 7);
        return FVM_OK;
    }
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = factory_result_mode == 2 ? 0 : factory_result_mode == 3 ? factory_foreign : factory_local;
    return factory_result_mode == 5 ? FVM_ERROR : factory_result_mode == 6 ? fvm_exception_raise(c, out) : FVM_OK;
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
        if (version && version < 4)
        {
            continue;
        }
        FVM_Context *c = 0, *other = 0;
        if (!(fvm_context_create(runtime, &c) == FVM_OK && fvm_context_create(runtime, &other) == FVM_OK))
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
        if (!(fvm_standard_install(c) == FVM_OK && fvm_object_create(c, &factory_local) == FVM_OK &&
              fvm_object_create(other, &factory_foreign) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *function = 0;
        if (!(fvm_function_create_native(c, factory_result, factory_local, factory_local, &function) == FVM_OK &&
              fvm_gc_pin(c, function) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        function->constructable = function->invocable;
        for (factory_result_mode = 1; factory_result_mode < 7; ++factory_result_mode)
        {
            FVM_Object *result = factory_local;
            FVM_Result expected = factory_result_mode == 4   ? FVM_OK
                                  : factory_result_mode == 6 ? FVM_EXCEPTION
                                                             : FVM_ERROR;
            if (!(fvm_function_construct(c, function, 0, 0, &result) == expected &&
                  result == (expected == FVM_OK ? factory_local : 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (expected == FVM_EXCEPTION)
            {
                FVM_Value payload;
                if (!(fvm_exception_get(c, &payload) == FVM_OK && payload.object == factory_local &&
                      fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(!c->gc_frames && !c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        factory_result_mode = 0;
        FVM_Object *result = factory_local;
        if (!(fvm_function_construct(c, function, 0, 0, &result) == FVM_ERROR && !result))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        fvm_context_destroy(other);
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
