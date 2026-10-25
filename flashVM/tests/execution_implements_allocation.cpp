/* Interface snapshot publication is atomic at every allocation boundary, including empty lists. */
#define main existing_allocation_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "../src/core/runtime.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned empty = 0; empty < 2; ++empty)
    {
        FVM_Context *c = 0;
        FVM_Object *klass = 0, *iface = 0, *proto = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
              fvm_object_create(c, &klass) == FVM_OK && fvm_gc_pin(c, klass) == FVM_OK &&
              fvm_object_create(c, &iface) == FVM_OK && fvm_gc_pin(c, iface) == FVM_OK &&
              fvm_object_create(c, &proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value type, arg, v;
        fvm_value_undefined(&type);
        type.type = FVM_VALUE_OBJECT;
        type.object = klass;
        arg = type;
        arg.object = iface;
        v = arg;
        v.object = proto;
        if (!(fvm_object_set(c, iface, "prototype", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (empty)
        {
            fvm_value_number(&arg, 17);
        }
        bool success = false;
        unsigned failures = 0;
        for (long boundary = 0; boundary < 2000 && !success; ++boundary)
        {
            FVM_Object *target = 0;
            if (!(fvm_object_create(c, &target) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            v.object = target;
            if (!(fvm_object_set(c, klass, "prototype", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_GCRootFrame *roots = c->gc_frames;
            allowance = boundary;
            FVM_Result r = fvm_exec_implements(c, &type, 1, &arg);
            allowance = -1;
            if (!(c->gc_frames == roots))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (r == FVM_OK)
            {
                if (!(target->interfaces_are_prototypes && target->implemented_interfaces.size() == (empty ? 0U : 1U)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!empty)
                {
                    if (!(target->implemented_interfaces[0] == proto))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                success = true;
            }
            else
            {
                if (!(r == FVM_OUT_OF_MEMORY && !target->interfaces_are_prototypes &&
                      target->implemented_interfaces.empty()))
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
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
