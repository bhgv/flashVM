/* C89 loaded InstanceOf uses own stored prototype data, without requiring a callable class. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"


static FVM_Result forbidden(FVM_Context *c, FVM_Object *o, unsigned n, const FVM_Value *a, FVM_Value *out, void *raw)
{
    (void)c;
    (void)o;
    (void)n;
    (void)a;
    (void)out;
    (void)raw;
    if (!(0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
}


static void object_value(FVM_Value *v, FVM_Object *o)
{
    fvm_value_undefined(v);
    v->type = FVM_VALUE_OBJECT;
    v->object = o;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned version;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (version = 0; version <= 8; ++version)
    {
        FVM_Context *c;
        FVM_Object *value, *target, *klass, *parent, *inherited, *accessor, *upper;
        FVM_Value input, type, proto, out;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (version && version < 4)
        {
            continue;
        }
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK && fvm_object_create(c, &value) == FVM_OK &&
              fvm_object_create(c, &target) == FVM_OK && fvm_object_create(c, &klass) == FVM_OK &&
              fvm_object_create(c, &parent) == FVM_OK && fvm_object_create(c, &inherited) == FVM_OK &&
              fvm_object_create(c, &accessor) == FVM_OK && fvm_object_create(c, &upper) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_prototype(c, value, target) == FVM_OK &&
              fvm_object_set_prototype(c, inherited, parent) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        object_value(&input, value);
        object_value(&proto, target);
        if (!(fvm_object_set(c, klass, "prototype", &proto) == FVM_OK &&
              fvm_object_set(c, parent, "prototype", &proto) == FVM_OK &&
              fvm_object_set(c, upper, "PROTOTYPE", &proto) == FVM_OK &&
              fvm_object_define_accessor_rooted(c, accessor, "prototype", forbidden, 0, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        object_value(&type, klass);
        if (!version)
        {
            fvm_value_number(&out, 99);
            if (!(fvm_exec_instance_of(c, &input, &type, &out) == FVM_UNSUPPORTED && out.number == 99))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_get_global(c, "Object", &type) == FVM_OK && fvm_construct(c, "Object", 0, 0, &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            object_value(&input, value);
            if (!(fvm_exec_instance_of(c, &input, &type, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
            continue;
        }
        if (!(fvm_exec_instance_of(c, &input, &type, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN && out.boolean &&
              fvm_exec_cast(c, &input, &type, &out) == FVM_OK && out.object == value))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        object_value(&type, inherited);
        if (!(fvm_exec_instance_of(c, &input, &type, &out) == FVM_OK && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        object_value(&type, accessor);
        if (!(fvm_exec_instance_of(c, &input, &type, &out) == FVM_OK && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        object_value(&type, upper);
        if (!(fvm_exec_instance_of(c, &input, &type, &type) == FVM_OK && type.boolean == (version < 7)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&type, 17);
        if (!(fvm_exec_instance_of(c, &input, &type, &out) == FVM_OK && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&input, 42);
        fvm_value_undefined(&type);
        if (!(fvm_exec_instance_of(c, &input, &type, &input) == FVM_OK && input.type == FVM_VALUE_BOOLEAN &&
              !input.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&out, 99);
        if (!(fvm_exec_instance_of(c, 0, &type, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
