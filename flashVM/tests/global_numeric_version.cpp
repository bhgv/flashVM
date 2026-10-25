#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include <cmath>


static void check(FVM_Context *c, FVM_Object *o, unsigned version)
{
    FVM_Value value;
    if (!(fvm_object_get(c, o, "NaN", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(version == 4 ? value.type == FVM_VALUE_UNDEFINED
                       : value.type == FVM_VALUE_NUMBER && value.number != value.number))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, o, "Infinity", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(version == 4 ? value.type == FVM_VALUE_UNDEFINED
                       : value.type == FVM_VALUE_NUMBER && value.number == HUGE_VAL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void attributes(FVM_Context *c, FVM_Object *o)
{
    const char *names[] = {"NaN", "Infinity"};
    for (unsigned i = 0; i < 2; ++i)
    {
        unsigned flags = 0;
        int own = 0;
        if (!(fvm_object_property_flags(c, o, names[i], &flags) == FVM_OK && flags == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_has_own(c, o, names[i], &own) == FVM_OK && own))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Object *keys = 0;
    FVM_Value length, key;
    if (!(fvm_object_enumerate(c, o, 0, &keys) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, keys, "length", &length) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned long i = 0; i < (unsigned long)length.number; ++i)
    {
        if (!(fvm_table_get_index(c, keys, i, &key) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(std::strcmp(key.string, "NaN") && std::strcmp(key.string, "Infinity")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        for (unsigned before = 0; before < 2; ++before)
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
            unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
            swf[3] = (unsigned char)(version ? version : 4);
            if (before)
            {
                if (!(fvm_standard_install(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (version)
            {
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
            check(c, c->global, version);
            attributes(c, c->global);
            FVM_Value value;
            FVM_Object *child = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &child) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string_bytes(&value, "a\0b", 3);
            if (!(fvm_object_set(c, child, "marker", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_set_global(c, "Infinity", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = object(child);
            if (!(fvm_set_global(c, "NaN", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
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
            if (version)
            {
                check(c, c->global, version);
            }
            else
            {
                if (!(fvm_get_global(c, "NaN", &value) == FVM_OK && value.object == child))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(c, child, "marker", &value) == FVM_OK && fvm_value_string_size(&value) == 3 &&
                      !std::memcmp(value.string, "a\0b", 3)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_get_global(c, "Infinity", &value) == FVM_OK && fvm_value_string_size(&value) == 3 &&
                      !std::memcmp(value.string, "a\0b", 3)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            FVM_Object *holder = 0;
            if (!(fvm_object_create(c, &holder) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(c, holder) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            holder->accessors["NaN"] = c->global->accessors.find("NaN")->second;
            holder->accessors["Infinity"] = c->global->accessors.find("Infinity")->second;
            holder->attributes["NaN"] = holder->attributes["Infinity"] = 1;
            FVM_Channel *channel = 0;
            if (!(fvm_channel_create(&channel) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = object(holder);
            if (!(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_channel_receive_value(other, channel, &value, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *copy = value.object;
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
            attributes(other, copy);
            if (!version)
            {
                if (!(fvm_object_get(other, copy, "NaN", &value) == FVM_OK && value.type == FVM_VALUE_OBJECT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(other, value.object, "marker", &value) == FVM_OK &&
                      fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(other, copy, "Infinity", &value) == FVM_OK && fvm_value_string_size(&value) == 3 &&
                      !std::memcmp(value.string, "a\0b", 3)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                check(other, copy, 0);
            }
            if (!(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            check(other, copy, version ? version : 4);
            int deleted = 0;
            if (!(fvm_object_set_property_flags(other, copy, "NaN", 2, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_delete(other, copy, "NaN", &deleted) == FVM_OK && !deleted))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_set_property_flags(other, copy, "NaN", 0, 3) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *keys = 0;
            if (!(fvm_object_enumerate(other, copy, 0, &keys) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(other, keys, "length", 1);
            if (!(fvm_table_get_index(other, keys, 0, &value) == FVM_OK && !std::strcmp(value.string, "NaN")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_delete(other, copy, "NaN", &deleted) == FVM_OK && deleted))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(other, copy, "NaN", &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_delete(other, copy, "Infinity", &deleted) == FVM_OK && deleted))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 81);
            if (!(fvm_object_set(other, copy, "NaN", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(other, copy, "NaN", 81);
            if (!(fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(other, copy, "NaN", 81);
            if (!(c->gc_frames == 0 && other->gc_frames == 0 && !c->avm1_execution_depth &&
                  !other->avm1_execution_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_channel_destroy(channel);
            fvm_context_destroy(other);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
