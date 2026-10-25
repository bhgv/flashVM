/* Class activation keeps edited flags/values, accessors, deleted slots and primitive metadata. */
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
        for (unsigned field = 0; field < 8; ++field)
        {
            for (unsigned kind = 0; kind < 5; ++kind)
            {
                if (field >= 5 && kind == 4)
                {
                    continue;
                }
                FVM_Context *c = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *klass = static_class(c, field < 5 ? 0 : field - 4);
                static_prototype(c, klass, kind == 4 ? 0 : 4);
                FVM_Value original, value;
                if (!(fvm_object_get(c, klass, static_fields[field], &original) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!kind)
                {
                    if (!(fvm_object_set_property_flags(c, klass, static_fields[field], 0, 7) == FVM_OK &&
                          fvm_object_set_property_flags(c, klass, static_fields[field], 2, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (kind == 1)
                {
                    fvm_value_number(&original, 81);
                    if (!(fvm_object_set(c, klass, static_fields[field], &original) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (kind == 2 || kind == 3)
                {
                    int removed = 0;
                    if (!(fvm_object_delete(c, klass, static_fields[field], &removed) == FVM_OK && removed))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (kind == 3)
                {
                    if (!(fvm_object_define_accessor_rooted(c, klass, static_fields[field], static_getter, 0, 0, 0) ==
                          FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (kind == 4)
                {
                    FVM_Object *meta = 0;
                    if (!(fvm_object_create(c, &meta) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    original.metatable = meta;
                    if (!(fvm_object_set(c, klass, static_fields[field], &original) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                unsigned expected = 0;
                if (!(fvm_object_property_flags(c, klass, static_fields[field], &expected) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                static_reads = 0;
                static_load(c, version);
                if (!(!static_reads && fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                math_flag(c, klass, static_fields[field], expected);
                if (kind == 2)
                {
                    int own = 1;
                    if (!(fvm_object_has_own(c, klass, static_fields[field], &own) == FVM_OK && !own))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (kind != 3)
                {
                    if (!(fvm_object_get(c, klass, static_fields[field], &value) == FVM_OK &&
                          value.type == original.type && value.metatable == original.metatable))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (value.type == FVM_VALUE_NUMBER)
                    {
                        if (!(value.number == original.number ||
                              (value.number != value.number && original.number != original.number)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                if (!(!c->gc_frames && !c->gc_native_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
