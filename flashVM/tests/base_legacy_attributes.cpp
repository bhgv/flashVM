#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"


static void flags(FVM_Context *c, FVM_Object *o, const char *name, unsigned expected)
{
    unsigned actual = 99;
    if (!(fvm_object_property_flags(c, o, name, &actual) == FVM_OK && actual == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *classes[] = {"Object", "Array", "String"}, *methods[] = {"valueOf", "push", "charAt"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned before = 0; before < 2; ++before)
        {
            for (unsigned custom = 0; custom <= before; ++custom)
            {
                for (unsigned kind = 0; kind < 3; ++kind)
                {
                    FVM_Context *c = 0, *other = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_context_create(rt, &other) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value klass, value, out;
                    FVM_Object *proto = 0;
                    const char *static_name = kind == 0 ? "registerClass" : "fromCharCode";
                    unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
                    swf[3] = (unsigned char)version;
                    if (before)
                    {
                        if (!(fvm_standard_install(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_get_global(c, classes[kind], &klass) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        proto = value.object;
                        flags(c, proto, methods[kind], 1);
                        if (kind != 1)
                        {
                            flags(c, klass.object, static_name, 1);
                        }
                        if (custom)
                        {
                            if (!(fvm_object_set_property_flags(c, proto, methods[kind], 0, 7) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (kind != 1)
                            {
                                if (!(fvm_object_set_property_flags(c, klass.object, static_name, 0, 7) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                    }
                    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_standard_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_get_global(c, classes[kind], &klass) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_get(c, klass.object, "prototype", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (before)
                    {
                        if (!(value.object == proto))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        proto = value.object;
                    }
                    flags(c, proto, methods[kind], custom ? 0 : 3);
                    flags(c, proto, "toString", 3);
                    int deleted = 1;
                    if (!(fvm_object_delete(c, proto, "toString", &deleted) == FVM_OK && !deleted))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_number(&value, 81);
                    if (!(fvm_object_set(c, proto, methods[kind], &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_get(c, proto, methods[kind], &out) == FVM_OK && out.number == 81))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_delete(c, proto, methods[kind], &deleted) == FVM_OK && !!deleted == !!custom))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (kind != 1)
                    {
                        unsigned expected = custom ? 0 : kind == 0 ? 7 : 3;
                        flags(c, klass.object, static_name, expected);
                        if (!(fvm_object_delete(c, klass.object, static_name, &deleted) == FVM_OK &&
                              !!deleted == !!custom))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!custom)
                        {
                            if (!(fvm_object_set(c, klass.object, static_name, &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_object_get(c, klass.object, static_name, &out) == FVM_OK &&
                                  out.type == (kind == 0 ? FVM_VALUE_OBJECT : FVM_VALUE_NUMBER)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    if (!(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK))
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
                    if (!(fvm_channel_send_value(c, channel, &klass, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *copy = out.object;
                    if (!(fvm_gc_pin(other, copy) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_gc_collect(other, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_get(other, copy, "prototype", &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    flags(other, out.object, "toString", 3);
                    if (!(fvm_object_delete(other, out.object, "toString", &deleted) == FVM_OK && !deleted))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(c->gc_frames == 0 && other->gc_frames == 0 && !c->primitive_depth && !other->primitive_depth))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_channel_destroy(channel);
                    fvm_context_destroy(other);
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
