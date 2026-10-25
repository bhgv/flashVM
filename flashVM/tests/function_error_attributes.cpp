/* Loaded Function adapters and script-defined Error.toString retain distinct property flags. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/serialization.h"

static unsigned attribute_reads;


static FVM_Result attribute_getter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++attribute_reads;
    fvm_value_number(out, 73);
    return FVM_OK;
}


static void check_attribute(FVM_Context *c, FVM_Object *object, const char *name, unsigned expected)
{
    unsigned flags = 99;
    if (!(fvm_object_property_flags(c, object, name, &flags) == FVM_OK && flags == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *keys = 0;
    if (!(fvm_object_enumerate(c, object, 0, &keys) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value length, key;
    if (!(fvm_object_get(c, keys, "length", &length) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool present = false;
    for (unsigned long i = 0; i < (unsigned long)length.number; ++i)
    {
        if (!(fvm_table_get_index(c, keys, i, &key) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (key.type == FVM_VALUE_STRING && !std::strcmp(key.string, name))
        {
            present = true;
        }
    }
    if (!(present == !(expected & FVM_PROPERTY_DONT_ENUM)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    const char *classes[] = {"Function", "Function", "Error"};
    const char *names[] = {"call", "apply", "toString"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned before = 0; before < 2; ++before)
        {
            for (unsigned kind = 0; kind < (before ? 7U : 1U); ++kind)
            {
                for (unsigned type = 0; type < 3; ++type)
                {
                    FVM_Context *c = 0, *other = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                    swf[3] = (unsigned char)version;
                    FVM_Value klass, value, proto;
                    unsigned expected = type == 2 ? 0 : 3;
                    FVM_Object *original = 0;
                    if (before)
                    {
                        if (!(fvm_standard_install(c) == FVM_OK && fvm_get_global(c, classes[type], &klass) == FVM_OK &&
                              fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        original = proto.object;
                        if (!(fvm_gc_pin(c, original) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        check_attribute(c, original, names[type], 1);
                        if (kind == 1 || kind == 2)
                        {
                            if (!(fvm_object_set_property_flags(
                                      c, original, names[type], kind == 1 ? 0 : 7, kind == 1 ? 7 : 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind == 3)
                        {
                            fvm_value_number(&value, 64);
                            if (!(fvm_object_set(c, original, names[type], &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind == 4)
                        {
                            int removed = 0;
                            if (!(fvm_object_delete(c, original, names[type], &removed) == FVM_OK && removed))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind == 5)
                        {
                            if (!(fvm_object_define_accessor_rooted(
                                      c, original, names[type], attribute_getter, 0, 0, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind == 6)
                        {
                            FVM_Object *replacement = 0;
                            if (!(fvm_object_create(c, &replacement) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            value = class_value(replacement);
                            if (!(fvm_object_set(c, klass.object, "prototype", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind && kind != 4)
                        {
                            if (!(fvm_object_property_flags(c, original, names[type], &expected) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    attribute_reads = 0;
                    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!before)
                    {
                        if (!(fvm_standard_install(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!attribute_reads))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!before)
                    {
                        if (!(fvm_get_global(c, classes[type], &klass) == FVM_OK &&
                              fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        original = proto.object;
                        if (!(fvm_gc_pin(c, original) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 4)
                    {
                        int own = 1;
                        if (!(fvm_object_has_own(c, original, names[type], &own) == FVM_OK && !own))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        check_attribute(c, original, names[type], expected);
                        if (kind != 5)
                        {
                            unsigned char *data = 0;
                            size_t size = 0;
                            value = class_value(original);
                            if (!(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK &&
                                  fvm_value_serialize(c, &value, &data, &size) == FVM_OK &&
                                  fvm_value_deserialize(other, data, size, &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            fvm_serialized_free(data);
                            if (!(fvm_gc_pin(other, value.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            check_attribute(other, value.object, names[type], expected);
                        }
                        if (!kind)
                        {
                            FVM_Value written, read;
                            fvm_value_number(&written, 84);
                            if (!(fvm_object_set(c, original, names[type], &written) == FVM_OK &&
                                  fvm_object_get(c, original, names[type], &read) == FVM_OK &&
                                  read.type == FVM_VALUE_NUMBER && read.number == 84))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            check_attribute(c, original, names[type], expected);
                        }
                        int removed = 0;
                        if (!(fvm_object_delete(c, original, names[type], &removed) == FVM_OK &&
                              !!removed == !(expected & FVM_PROPERTY_DONT_DELETE)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!attribute_reads && !c->gc_frames && !other->gc_frames))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(other);
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
