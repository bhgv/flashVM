/* Script constructor prototypes retain original Object methods after replacement and GC. */
#define main named_scope_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_function2_named_scope.cpp"

#undef main
#include "flashvm/property.h"


static FVM_Object *make(FVM_Context *c)
{
    unsigned char body[] = {0};
    FVM_Object *fn = 0;
    if (!(fvm_function_create_avm1(c, body, sizeof(body), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fn;
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
    for (unsigned version = 5; version <= 9; ++version)
    {
        FVM_Context *c = 0, *other = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version <= 8)
        {
            unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
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
        FVM_Value klass, original;
        if (!(fvm_get_global(c, "Object", &klass) == FVM_OK &&
              fvm_object_get(c, klass.object, "prototype", &original) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *replacement = 0;
        if (!(fvm_object_create(c, &replacement) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value, out;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = replacement;
        if (!(fvm_object_set(c, klass.object, "prototype", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, klass.object, "prototype", &out) == FVM_OK && out.object == replacement))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *fn = make(c);
        if (!(fvm_gc_pin(c, fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value proto;
        if (!(fvm_object_get(c, fn, "prototype", &proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(proto.object->prototype == (version <= 8 ? original.object : 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version <= 8)
        {
            unsigned flags = 0;
            int deleted = 1;
            if (!(fvm_object_property_flags(c, fn, "prototype", &flags) == FVM_OK && flags == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_delete(c, fn, "prototype", &deleted) == FVM_OK && !deleted))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Bytes body;
            body.push_back(0x96);
            u16(body, 5);
            body.push_back(7);
            body.push_back(77);
            body.push_back(0);
            body.push_back(0);
            body.push_back(0);
            body.push_back(0x3e);
            body.push_back(0);
            FVM_Object *getter = 0;
            if (!(fvm_function_create_avm1(c, body.begin(), body.size(), 0, 0, 0, &getter) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value args[3];
            fvm_value_string(args, "answer");
            fvm_value_undefined(args + 1);
            args[1].type = FVM_VALUE_OBJECT;
            args[1].object = getter;
            fvm_value_undefined(args + 2);
            args[2].type = FVM_VALUE_NULL;
            if (!(fvm_object_call(c, proto.object, "addProperty", 3, args, &out) == FVM_OK &&
                  out.type == FVM_VALUE_BOOLEAN && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *instance = 0;
            if (!(fvm_function_construct(c, fn, 0, 0, &instance) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, instance, "answer", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
                  out.number == 77))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, proto.object, "answer", &out) == FVM_OK &&
                  out.number == 77))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Channel *channel = 0;
            if (!(fvm_channel_create(&channel) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value.object = fn;
            if (!(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK &&
                  fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *copy = out.object;
            if (!(fvm_object_property_flags(other, copy, "prototype", &flags) == FVM_OK && flags == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_construct(other, copy, 0, 0, &instance) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(other, instance, "answer", &out) == FVM_OK && out.number == 77))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_channel_destroy(channel);
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
