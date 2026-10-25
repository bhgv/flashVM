/* Recreated and virtual length writes unwind every allocation boundary and remain retryable. */
#define main reassignment_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define FVM_REASSIGNMENT_FIXTURE_ONLY
#include "array_length_reassignment.cpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned kind = 0; kind < 5; ++kind)
    {
        for (unsigned object = 0; object < 2; ++object)
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
                version_load(c, 8);
                if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *array = reassignment_array(c, kind), *meta = 0;
                if (!(fvm_gc_pin(c, array) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value input = typed_length_input(c, object ? 8 : 0, meta), out;
                if (!(fvm_set_global(c, "reassignmentInput", &input) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                allowance = boundary;
                FVM_Result result = fvm_object_set(c, array, "length", &input);
                allowance = -1;
                if (result == FVM_OUT_OF_MEMORY)
                {
                    failed = true;
                    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_set(c, array, "length", &input) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(result == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    success = true;
                }
                out = reassignment_raw(c, array, kind);
                typed_length_equal(out, input);
                if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->calling_setter_owner))
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
