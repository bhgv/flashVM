/* Schema 9 restore allocation failures leave no partial objects or interface snapshot state. */
#define main existing_allocation_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "flashvm/serialization.h"
#include "../src/core/runtime.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *src = 0, *dst = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &src) == FVM_OK && fvm_context_create(rt, &dst) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    if (!(fvm_load_memory(src, swf, sizeof(swf)) == FVM_OK && fvm_load_memory(dst, swf, sizeof(swf)) == FVM_OK &&
          fvm_standard_install(src) == FVM_OK && fvm_standard_install(dst) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *klass = 0, *target = 0, *iface = 0, *proto = 0, *instance = 0;
    if (!(fvm_object_create(src, &klass) == FVM_OK && fvm_object_create(src, &target) == FVM_OK &&
          fvm_object_create(src, &iface) == FVM_OK && fvm_object_create(src, &proto) == FVM_OK &&
          fvm_object_create(src, &instance) == FVM_OK && fvm_object_set_prototype(src, instance, target) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value type, arg, v, root, out;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = target;
    if (!(fvm_object_set(src, klass, "prototype", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v.object = proto;
    if (!(fvm_object_set(src, iface, "prototype", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    type = v;
    type.object = klass;
    arg = v;
    arg.object = iface;
    fvm_value_number(&v, 17);
    if (!(fvm_object_set(src, proto, "alive", &v) == FVM_OK && fvm_exec_implements(src, &type, 1, &arg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&root);
    root.type = FVM_VALUE_OBJECT;
    root.object = instance;
    unsigned char *data = 0;
    size_t size = 0;
    if (!(fvm_value_serialize(src, &root, &data, &size) == FVM_OK && data[7] == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool success = false;
    unsigned failures = 0;
    for (long boundary = 0; boundary < 2000 && !success; ++boundary)
    {
        unsigned long before = 0, after = 0;
        if (!(fvm_gc_collect(dst, 0) == FVM_OK && fvm_gc_object_count(dst, &before) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_GCRootFrame *roots = dst->gc_frames;
        allowance = boundary;
        FVM_Result r = fvm_value_deserialize(dst, data, size, &out);
        allowance = -1;
        if (!(dst->gc_frames == roots))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (r == FVM_OK)
        {
            if (!(out.type == FVM_VALUE_OBJECT && fvm_gc_pin(dst, out.object) == FVM_OK &&
                  fvm_gc_collect(dst, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *p = out.object->prototype;
            if (!(p && p->interfaces_are_prototypes && p->implemented_interfaces.size() == 1 &&
                  fvm_object_get(dst, p->implemented_interfaces[0], "alive", &v) == FVM_OK && v.number == 17))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            success = true;
        }
        else
        {
            if (!(r == FVM_OUT_OF_MEMORY && out.type == FVM_VALUE_UNDEFINED &&
                  fvm_gc_object_count(dst, &after) == FVM_OK && after == before))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ++failures;
        }
    }
    if (!(success && failures))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(data);
    fvm_context_destroy(dst);
    fvm_context_destroy(src);
    fvm_runtime_destroy(rt);
    return 0;
}
