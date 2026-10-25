/* OOM cleanup and retry must not revive deleted Array length or evaluate element getters. */
#define main deleted_length_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define FVM_APPLY_DELETED_LENGTH_FIXTURE_ONLY
#include "function_apply_deleted_length.cpp"

static unsigned deleted_allocation_index_calls;


static FVM_Result deleted_allocation_index(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *, void *)
{
    ++deleted_allocation_index_calls;
    return FVM_ERROR;
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
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary)
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
            FVM_Object *array = deleted_length_array(c), *fn = deleted_length_function(c),
                       *getter = deleted_length_function(c);
            if (!(fvm_gc_pin(c, array) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK &&
                  fvm_function_set_body(c, getter, deleted_allocation_index, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value args[3], out;
            fvm_value_string(args, "0");
            args[1] = class_value(getter);
            fvm_value_undefined(args + 2);
            args[2].type = FVM_VALUE_NULL;
            if (!(fvm_object_call(c, array, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(args);
            args[0].type = FVM_VALUE_NULL;
            args[1] = class_value(array);
            deleted_allocation_index_calls = 0;
            allowance = boundary;
            const FVM_Result status = fvm_object_call(c, fn, "apply", 2, args, &out);
            allowance = -1;
            if (!(!deleted_allocation_index_calls && !c->gc_frames && !c->gc_native_depth && !c->script_depth &&
                  !c->exception_pending && !c->script_this_cell))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (status == FVM_OUT_OF_MEMORY)
            {
                failed = true;
                if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_call(c, fn, "apply", 2, args, &out) == FVM_OK))
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
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 0 && !deleted_allocation_index_calls))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            int own = 1;
            if (!(fvm_object_has_own(c, array, "length", &own) == FVM_OK && !own))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_table_get_index(c, array, 1, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 22))
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
