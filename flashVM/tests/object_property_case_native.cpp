/* Rooted C definitions use the same stored spelling and conflict policy as loaded lookup. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"


static FVM_Result native_value(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *data)
{
    return fvm_object_get(c, (FVM_Object *)data, "value", out);
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
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
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const bool folded = version && version <= 6;
        FVM_Object *object = 0, *one = 0, *two = 0;
        FVM_Value value, old, out;
        if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &one) == FVM_OK && fvm_object_create(c, &two) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 1);
        if (!(fvm_object_set(c, one, "value", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 2);
        if (!(fvm_object_set(c, two, "value", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted_bytes(c, object, "MiXeD\0Name", 10, native_value, one, one) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get_bytes(c, object, "MiXeD\0Name", 10, &old) == FVM_OK && old.type == FVM_VALUE_OBJECT &&
              fvm_gc_pin(c, old.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted_bytes(c, object, "mixed\0name", 10, native_value, two, two) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(object->methods.size() == (folded ? 1U : 2U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call_bytes(c, object, "MiXeD\0Name", 10, 0, 0, &out) == FVM_OK &&
              out.number == (folded ? 2 : 1)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call_bytes(c, object, "mixed\0name", 10, 0, 0, &out) == FVM_OK && out.number == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_invoke(c, old.object, object, 0, 0, &out) == FVM_OK && out.number == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned accessor = 0; accessor < 2; ++accessor)
        {
            FVM_Object *target = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &target) == FVM_OK && fvm_gc_pin(c, target) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 9);
            if (!(fvm_object_set_bytes(c, target, "DaTa\0Name", 9, &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Result status =
                accessor
                    ? fvm_object_define_accessor_rooted_bytes(c, target, "data\0name", 9, native_value, 0, two, two)
                    : fvm_object_define_method_rooted_bytes(c, target, "data\0name", 9, native_value, two, two);
            if (!(status == (folded ? FVM_INVALID_ARGUMENT : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get_bytes(c, target, "DaTa\0Name", 9, &out) == FVM_OK && out.number == 9))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_Object *accessor = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &accessor) == FVM_OK && fvm_gc_pin(c, accessor) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_accessor_rooted(c, accessor, "Getter", native_value, 0, one, one) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_accessor_rooted(c, accessor, "getter", native_value, 0, two, two) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(accessor->accessors.size() == (folded ? 1U : 2U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, accessor, "Getter", &out) == FVM_OK &&
              out.number == (folded ? 2 : 1)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
