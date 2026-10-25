/* Loaded interface declarations capture prototype identity once and respect SWF7 gating. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"


static void obj(FVM_Value *v, FVM_Object *o)
{
    fvm_value_undefined(v);
    v->type = FVM_VALUE_OBJECT;
    v->object = o;
}


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
    for (version = 4; version <= 8; ++version)
    {
        FVM_Context *c;
        FVM_Object *klass, *target, *iface, *old, *replacement, *second, *second_proto, *instance, *alias, *empty_class,
            *empty_target, *empty_instance, *accessor;
        FVM_Value type, args[2], v, input, out;
        unsigned n = 99;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK && fvm_object_create(c, &klass) == FVM_OK &&
              fvm_object_create(c, &target) == FVM_OK && fvm_object_create(c, &iface) == FVM_OK &&
              fvm_object_create(c, &old) == FVM_OK && fvm_object_create(c, &replacement) == FVM_OK &&
              fvm_object_create(c, &second) == FVM_OK && fvm_object_create(c, &second_proto) == FVM_OK &&
              fvm_object_create(c, &instance) == FVM_OK && fvm_object_create(c, &accessor) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(&v, target);
        if (!(fvm_object_set(c, klass, "prototype", &v) == FVM_OK && fvm_gc_pin(c, klass) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(&v, old);
        if (!(fvm_object_set(c, iface, "prototype", &v) == FVM_OK && fvm_gc_pin(c, iface) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 17);
        if (!(fvm_object_set(c, old, "alive", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(&v, second_proto);
        if (!(fvm_object_set(c, second, "prototype", &v) == FVM_OK && fvm_gc_pin(c, second) == FVM_OK &&
              fvm_object_set_prototype(c, instance, target) == FVM_OK && fvm_gc_pin(c, instance) == FVM_OK &&
              fvm_gc_pin(c, accessor) == FVM_OK &&
              fvm_object_define_accessor_rooted(c, accessor, "prototype", forbidden, 0, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(&type, klass);
        obj(args, accessor);
        obj(args + 1, iface);
        if (!(fvm_exec_implements(c, &type, 0, 0) == FVM_OK && fvm_exec_implements(c, &type, 2, args) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(&input, instance);
        obj(&v, iface);
        if (!(fvm_exec_instance_of(c, &input, &v, &out) == FVM_OK && out.boolean == (version >= 7)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(&v, replacement);
        if (!(fvm_object_set(c, iface, "prototype", &v) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(&v, iface);
        if (!(fvm_exec_instance_of(c, &input, &v, &out) == FVM_OK && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version >= 7)
        {
            if (!(fvm_object_get(c, old, "alive", &out) == FVM_OK && out.number == 17 &&
                  fvm_object_create(c, &alias) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            obj(&v, old);
            if (!(fvm_object_set(c, alias, "prototype", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            obj(&v, alias);
            if (!(fvm_exec_instance_of(c, &input, &v, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        obj(args, second);
        if (!(fvm_exec_implements(c, &type, 1, args) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(&v, second);
        if (!(fvm_exec_instance_of(c, &input, &v, &out) == FVM_OK && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &empty_class) == FVM_OK && fvm_object_create(c, &empty_target) == FVM_OK &&
              fvm_object_create(c, &empty_instance) == FVM_OK &&
              fvm_object_set_prototype(c, empty_instance, empty_target) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(&v, empty_target);
        if (!(fvm_object_set(c, empty_class, "prototype", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(&type, empty_class);
        fvm_value_number(args, 17);
        if (!(fvm_exec_implements(c, &type, 1, args) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(args, second);
        if (!(fvm_exec_implements(c, &type, 1, args) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(&input, empty_instance);
        obj(&v, second);
        if (!(fvm_exec_instance_of(c, &input, &v, &out) == FVM_OK && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, -1);
        if (!(fvm_exec_interface_count(c, &v, 3, &n) == FVM_OK && n == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 1.9);
        if (!(fvm_exec_interface_count(c, &v, 3, &n) == FVM_OK && n == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, klass, "valueOf", forbidden, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        obj(&v, klass);
        if (!(fvm_exec_interface_count(c, &v, 3, &n) == FVM_OK && n == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 65536);
        if (!(fvm_exec_interface_count(c, &v, 65536, &n) == FVM_OK && n == 65536))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
