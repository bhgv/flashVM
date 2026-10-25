#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/property.h"
#include "flashvm/transfer.h"
#include "flashvm/execution.h"
#include "../src/core/runtime.hpp"
#include <cstring>


static FVM_Value reference(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = o;
    return v;
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
    for (unsigned version = 4; version <= 9; ++version)
    {
        FVM_Context *c = 0, *other = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version <= 8)
        {
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *o = 0, *parent = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK && fvm_gc_pin(c, o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK && fvm_gc_pin(c, parent) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value, key, out;
        fvm_value_string(&key, "__proto__");
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, parent, "inherited", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = reference(parent);
        if (!(fvm_object_set(c, o, "__proto__", &value) == FVM_OK && o->prototype == parent))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, o, "inherited", &out) == FVM_OK && out.number == 17))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const char raw_bytes[] = {'a', 0, 'b'};
        for (unsigned kind = 0; kind < 5; ++kind)
        {
            if (kind == 0)
            {
                fvm_value_number(&value, 19);
            }
            else if (kind == 1)
            {
                fvm_value_boolean(&value, 1);
            }
            else if (kind == 2)
            {
                fvm_value_string_bytes(&value, raw_bytes, 3);
            }
            else
            {
                fvm_value_undefined(&value);
                if (kind == 3)
                {
                    value.type = FVM_VALUE_NULL;
                }
            }
            FVM_Value owner = reference(o);
            if (!(fvm_exec_member(c, 1, &owner, &key, &value, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version == 9)
            {
                if (!(o->prototype == (kind < 3 ? parent : 0)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                continue;
            }
            if (!(!o->prototype && fvm_object_get(c, o, "__proto__", &out) == FVM_OK && out.type == value.type))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (kind == 2)
            {
                if (!(out.string_size == 3 && !std::memcmp(out.string, raw_bytes, 3)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (kind == 2)
            {
                FVM_Channel *ch = 0;
                FVM_Value sent = reference(o), received, raw;
                if (!(fvm_channel_create(&ch) == FVM_OK && fvm_channel_send_value(c, ch, &sent, 0) == FVM_OK &&
                      fvm_channel_receive_value(other, ch, &received, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(other, received.object, "__proto__", &raw) == FVM_OK &&
                      raw.type == FVM_VALUE_STRING && raw.string_size == 3 && !std::memcmp(raw.string, raw_bytes, 3)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_channel_destroy(ch);
            }
            if (kind == 0)
            {
                if (!(out.number == 19))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_object_get(c, o, "inherited", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            int own = 0;
            if (!(fvm_object_has_own(c, o, "__proto__", &own) == FVM_OK && own))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned flags = 0;
            if (!(fvm_object_property_flags(c, o, "__proto__", &flags) == FVM_OK && flags == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, o, "__proto__", &out) == FVM_OK &&
                  out.type == value.type))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (version <= 8)
        {
            if (!(fvm_object_set_prototype(c, o, parent) == FVM_OK &&
                  fvm_object_get(c, o, "__proto__", &out) == FVM_OK && out.object == parent))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = reference(o);
            if (!(fvm_object_set(c, o, "__proto__", &value) == FVM_INVALID_ARGUMENT && o->prototype == parent))
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
            if (!(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK &&
                  fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(other, out.object, "inherited", &value) == FVM_OK && value.number == 17))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_channel_destroy(channel);
            if (!(fvm_object_set_property_flags(c, o, "__proto__", 0, 3) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 31);
            if (!(fvm_object_set(c, o, "__proto__", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned flags = 3;
            if (!(fvm_object_property_flags(c, o, "__proto__", &flags) == FVM_OK && flags == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            int deleted = 0;
            if (!(fvm_object_delete(c, o, "__proto__", &deleted) == FVM_OK && deleted && !o->prototype))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, o, "__proto__", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
