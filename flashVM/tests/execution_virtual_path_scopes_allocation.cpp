/* Getter-free virtual membership preserves operation roots and chooses the correct binding owner. */
#define main existing_allocation_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "../src/core/runtime.hpp"
#include "flashvm/property.h"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *root = 0, *child = 0, *activation = 0, *scope = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
          fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK &&
          fvm_object_create(c, &activation) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value args[2], value, key, out;
    fvm_value_string(args, "child");
    fvm_value_number(args + 1, 1);
    if (!(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    child = out.object;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = child;
    if (!(fvm_exec_with_scope(c, activation, &value, &scope) == FVM_OK && fvm_gc_pin(c, scope) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&key, "_root");
    fvm_value_number(&value, 99);
    if (!(fvm_object_set(c, root, "_root", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 4; ++mode)
    {
        bool success = false;
        unsigned failures = 0;
        for (long boundary = 0; boundary < 2000 && !success; ++boundary)
        {
            int own = 0, removed = 0;
            if (!(fvm_object_has_own(c, child, "_root", &own) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (own)
            {
                if (!(fvm_object_delete(c, child, "_root", &removed) == FVM_OK && removed))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_value_number(&value, 99);
            if (!(fvm_object_set(c, activation, "_root", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 17);
            FVM_GCRootFrame *roots = c->gc_frames;
            fvm_value_number(&out, 99);
            allowance = boundary;
            FVM_Result r = mode == 0   ? fvm_exec_variable_set(c, scope, root, child, &key, &value)
                           : mode == 1 ? fvm_exec_scope_delete(c, scope, root, &key, &out)
                                       : fvm_exec_variable_declare(c,
                                                                   scope,
                                                                   activation,
                                                                   root,
                                                                   child,
                                                                   mode == 3 ? FVM_EXEC_DECLARE_INITIALIZED
                                                                             : FVM_EXEC_DECLARE_UNINITIALIZED,
                                                                   &key,
                                                                   mode == 3 ? &value : 0);
            allowance = -1;
            if (!(c->gc_frames == roots))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (r == FVM_OK)
            {
                if (mode == 0)
                {
                    if (!(fvm_object_get(c, child, "_root", &out) == FVM_OK && out.number == 17))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (mode == 1)
                {
                    if (!(out.type == FVM_VALUE_BOOLEAN && !out.boolean))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_object_get(c, activation, "_root", &out) == FVM_OK && out.number == (mode == 3 ? 17 : 99) &&
                      fvm_object_get(c, root, "_root", &out) == FVM_OK && out.number == 99))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                success = true;
            }
            else
            {
                if (!(r == FVM_OUT_OF_MEMORY && out.type == FVM_VALUE_NUMBER && out.number == 99))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                ++failures;
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(success && failures))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
