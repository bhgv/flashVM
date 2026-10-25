/* Replaced prototypes cannot suppress untouched builtin class metadata. */
#include <stdio.h>
#include <stdlib.h>
#include "static_activation_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned type = 0; type < 4; ++type)
        {
            for (unsigned variant = 0; variant < 7; ++variant)
            {
                for (unsigned route = 0; route < 4; ++route)
                {
                    if (route >= 2 && !fvm_jit_available())
                    {
                        continue;
                    }
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *klass = static_class(c, type);
                    static_prototype(c, klass, variant);
                    static_flags(c, klass, type, type ? 1U : 0U);
                    static_reads = 0;
                    static_load(c, version);
                    if (!(!static_reads))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    static_flags(c, klass, type, type == 2 ? 3U : 7U);
                    const unsigned begin = type ? type + 4 : 0, end = type ? type + 5 : 5;
                    FVM_Value target = class_value(klass), out;
                    if (!(fvm_set_global(c, "staticTarget", &target) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    for (unsigned field = begin; field < end; ++field)
                    {
                        FVM_Value original, value;
                        if (!(fvm_object_get(c, klass, static_fields[field], &original) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!route)
                        {
                            fvm_value_number(&value, 81);
                            if (!(fvm_object_set(c, klass, static_fields[field], &value) == FVM_OK &&
                                  fvm_object_get(c, klass, static_fields[field], &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            class_mode = route - 1;
                            Program code;
                            code.variable("staticTarget");
                            code.text(static_fields[field]);
                            code.number(81);
                            code.op(0x4f);
                            code.variable("staticTarget");
                            code.text(static_fields[field]);
                            code.op(0x4e);
                            code.finish();
                            out = class_run(c, code);
                        }
                        if (type == 2)
                        {
                            if (!(out.type == FVM_VALUE_NUMBER && out.number == 81))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(out.type == original.type))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (out.type == FVM_VALUE_NUMBER)
                            {
                                if (!(out.number == original.number ||
                                      (out.number != out.number && original.number != original.number)))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            else
                            {
                                if (!(out.object == original.object))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                        int removed = 1;
                        if (!(fvm_object_delete(c, klass, static_fields[field], &removed) == FVM_OK && !removed))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    static_flags(c, klass, type, type == 2 ? 3U : 7U);
                    if (!route && variant != 5)
                    {
                        FVM_Context *other = 0;
                        unsigned char *data = 0;
                        size_t bytes = 0;
                        if (!(fvm_context_create(rt, &other) == FVM_OK &&
                              fvm_value_serialize(c, &target, &data, &bytes) == FVM_OK &&
                              fvm_value_deserialize(other, data, bytes, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_serialized_free(data);
                        if (!(fvm_gc_pin(other, out.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        static_flags(other, out.object, type, type == 2 ? 3U : 7U);
                        fvm_context_destroy(other);
                    }
                    /* Explicitly clearing protection is a live edit, preserved on reload. */
                    for (unsigned field = begin; field < end; ++field)
                    {
                        if (!(fvm_object_set_property_flags(c, klass, static_fields[field], 0, 7) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    static_load(c, version);
                    static_flags(c, klass, type, 0);
                    if (!(!static_reads && !c->gc_frames && !c->gc_native_depth))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
