/* Fixed watcher depth restores invocation state after allocation failure and retry. */
#define main watch_depth_allocation_fixture_main
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
    const char key[] = "n\0m";
    for (unsigned version = 4; version <= 8; ++version)
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
            if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *receiver = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value args[3], out;
            fvm_value_string_bytes(args, key, sizeof(key) - 1);
            args[1] = class_value(adapter_super_function(c, 0));
            fvm_value_string_bytes(args + 2, "u\0d", 3);
            if (!(fvm_object_call(c, receiver, "watch", 3, args, &out) == FVM_OK && out.boolean &&
                  fvm_object_set_prototype(c, receiver, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value value;
            fvm_value_string_bytes(&value, "r\0s", 3);
            allowance = boundary;
            FVM_Result status = fvm_object_set_bytes(c, receiver, key, sizeof(key) - 1, &value);
            allowance = -1;
            if (!(c->watch_running.empty() && !c->gc_frames && !c->gc_native_depth && !c->calling_function &&
                  !c->calling_owner && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (status == FVM_OUT_OF_MEMORY)
            {
                failed = true;
                if (!(fvm_gc_collect(c, 0) == FVM_OK &&
                      fvm_object_set_bytes(c, receiver, key, sizeof(key) - 1, &value) == FVM_OK))
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
            if (!(fvm_object_get_bytes(c, receiver, key, sizeof(key) - 1, &out) == FVM_OK &&
                  out.type == FVM_VALUE_OBJECT && out.object->resource_result.object->properties["depth"].number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
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
    fvm_runtime_destroy(rt);
}
