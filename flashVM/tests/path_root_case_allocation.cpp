/* Root keyword and ordinary counted-prefix lookup preserve output atomicity across OOM. */
#define main root_case_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define FVM_PATH_ROOT_CASE_FIXTURE_ONLY
#include "path_root_case.cpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *keys[] = {"_root", "_ROOT", "_Root", "_ROOT\0suffix"};
    const size_t sizes[] = {5, 5, 5, 12};
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (unsigned key = 0; key < 4; ++key)
        {
            bool failed = false, success = false;
            for (long boundary = 0; boundary < 256 && !success; ++boundary)
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
                FVM_Object *owner = 0, *shadow = 0, *scope = 0;
                if (!(fvm_object_create(c, &owner) == FVM_OK && fvm_object_create(c, &shadow) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value value = class_value(shadow), path, out;
                if (!(fvm_object_set_bytes(c, owner, keys[key], sizes[key], &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                value = class_value(owner);
                if (!(fvm_exec_with_scope(c, 0, &value, &scope) == FVM_OK && fvm_gc_pin(c, scope) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_value_string_bytes(&path, keys[key], sizes[key]) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_number(&out, 77);
                allowance = boundary;
                const FVM_Result status = fvm_exec_scope_target(c, scope, c->global, c->global, &path, &out);
                allowance = -1;
                if (status == FVM_OUT_OF_MEMORY)
                {
                    failed = true;
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 77))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_gc_collect(c, 0) == FVM_OK &&
                          fvm_exec_scope_target(c, scope, c->global, c->global, &path, &out) == FVM_OK))
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
                if (!(out.type == FVM_VALUE_OBJECT && out.object == (!key ? c->global : shadow)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!c->gc_frames && !c->gc_native_depth && !c->script_this_cell))
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
