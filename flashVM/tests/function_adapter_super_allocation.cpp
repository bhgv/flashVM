/* Missing-prototype adapter facades survive OOM retry with depth state and roots restored. */
#define main adapter_super_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include "function_adapter_super.cpp"
#include "../src/core/call_scope_constants.h"


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
        for (unsigned apply = 0; apply < 2; ++apply)
        {
            bool failed = false, success = false;
            for (long boundary = 0; boundary < 1024 && !success; ++boundary)
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
                FVM_Object *receiver = 0;
                if (!(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK &&
                      fvm_object_set_prototype(c, receiver, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *function = adapter_super_function(c, 0);
                if (!(fvm_gc_pin(c, function) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value value = class_value(receiver), out;
                allowance = boundary;
                const FVM_Result status = fvm_object_call(c, function, apply ? "apply" : "call", 1, &value, &out);
                allowance = -1;
                if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner &&
                      !c->exception_pending && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (status == FVM_OUT_OF_MEMORY)
                {
                    failed = true;
                    if (!(fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_call(c, function, apply ? "apply" : "call", 1, &value, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(status == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    success = true;
                }
                if (!(out.type == FVM_VALUE_OBJECT && out.object->resource_kind == FVM_RESOURCE_SUPER &&
                      out.object->resource_result.object->properties["depth"].number == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *facade = out.object;
                if (!(fvm_gc_pin(c, facade) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(c, facade, "missing", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
            if (!(failed && success))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
