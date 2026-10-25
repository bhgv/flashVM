#define main existing_allocation_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *scope = 0, *child = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &scope) == FVM_OK && fvm_gc_pin(c, scope) == FVM_OK &&
          fvm_object_create(c, &child) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value, path, name, reference, out;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = child;
    if (!(fvm_object_set(c, scope, "child", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&path, "child");
    fvm_value_string(&name, "victim");
    fvm_value_string(&reference, "child.victim");
    fvm_value_number(&value, 17);
    for (unsigned mode = 0; mode < 2; ++mode)
    {
        bool success = false;
        unsigned failures = 0;
        for (long boundary = 0; boundary < 2000 && !success; ++boundary)
        {
            if (!(fvm_object_set(c, child, "victim", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_GCRootFrame *roots = c->gc_frames;
            fvm_value_number(&out, 99);
            int present = 99;
            allowance = boundary;
            FVM_Result r = mode ? fvm_exec_delete_reference(c, scope, 0, 0, &reference, &out)
                                : fvm_exec_scope_member(c, scope, 0, 0, &path, &name, &out, &present);
            allowance = -1;
            if (!(c->gc_frames == roots))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            int own = 0;
            if (!(fvm_object_has_own(c, child, "victim", &own) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (r == FVM_OK)
            {
                if (mode)
                {
                    if (!(out.boolean && !own))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(present && out.number == 17 && own))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                success = true;
            }
            else
            {
                if (!(r == FVM_OUT_OF_MEMORY && out.number == 99 && present == 99 && own))
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
        if (!(success && failures > 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
