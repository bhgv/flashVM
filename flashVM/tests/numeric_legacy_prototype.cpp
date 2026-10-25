#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"


static void generic(FVM_Context *c, FVM_Object *proto)
{
    FVM_Value out;
    if (!(proto->resource_kind == 30 && !proto->resource_result_valid))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, proto, "valueOf", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, proto, "toString", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
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
    const char *names[] = {"Number", "Boolean"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned before = 0; before < 2; ++before)
        {
            for (unsigned kind = 0; kind < 2; ++kind)
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
                FVM_Object *proto = 0, *host = 0;
                unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
                swf[3] = (unsigned char)version;
                if (before)
                {
                    if (!(fvm_standard_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_get_global(c, names[kind], &klass) == FVM_OK))
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
                    if (!(fvm_object_call(c, proto, "valueOf", 0, 0, &out) == FVM_OK &&
                          out.type == (kind ? FVM_VALUE_BOOLEAN : FVM_VALUE_NUMBER)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_number(&value, 81);
                    if (!(fvm_construct(c, names[kind], 1, &value, &host) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_gc_pin(c, host) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    const unsigned char invalid[] = {'F', 'W', 'S', 8};
                    if (!(fvm_load_memory(c, invalid, sizeof(invalid)) != FVM_OK && !c->swf_loaded))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_call(c, proto, "valueOf", 0, 0, &out) == FVM_OK &&
                          out.type == (kind ? FVM_VALUE_BOOLEAN : FVM_VALUE_NUMBER)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_string_bytes(&value, "x\0y", 3);
                    if (!(fvm_object_set(c, proto, "marker", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
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
                if (!(fvm_get_global(c, names[kind], &klass) == FVM_OK))
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
                generic(c, proto);
                if (!(fvm_gc_pin(c, proto) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (before)
                {
                    if (!(fvm_object_get(c, proto, "marker", &value) == FVM_OK && fvm_value_string_size(&value) == 3 &&
                          !std::memcmp(value.string, "x\0y", 3)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_call(c, host, "valueOf", 0, 0, &out) == FVM_OK &&
                          (kind ? out.boolean : out.number == 81)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                generic(c, proto);
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
                value = object(proto);
                if (!(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK))
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
                if (!(fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(other, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                generic(other, copy);
                if (!(fvm_object_get(other, copy, "constructor", &klass) == FVM_OK && klass.type == FVM_VALUE_OBJECT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *constructed = 0;
                fvm_value_number(&value, 81);
                if (!(fvm_function_construct(other, klass.object, 1, &value, &constructed) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_call(other, constructed, "valueOf", 0, 0, &out) == FVM_OK &&
                      (kind ? out.type == FVM_VALUE_BOOLEAN && out.boolean
                            : out.type == FVM_VALUE_NUMBER && out.number == 81)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get_prototype(other, constructed, &value.object) == FVM_OK && value.object == copy))
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
    fvm_runtime_destroy(rt);
}
