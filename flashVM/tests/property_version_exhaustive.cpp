/* Exhaustive UInt16 attributes: independent masks, ordered updates and write protection. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"


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
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        version_load(c, version);
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *child = 0, *parent = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &child) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_prototype(c, child, parent) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value, out;
        fvm_value_number(&value, 11);
        if (!(fvm_object_set(c, parent, "field", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = class_value(child);
        if (!(fvm_set_global(c, "exhaustiveTarget", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned flags = 0; flags <= 65535; ++flags)
        {
            version_flags(c, child, "field", 0, 65535);
            fvm_value_number(&value, 22);
            if (!(fvm_object_set(c, child, "field", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            version_flags(c, child, "field", flags, 65535);
            unsigned actual = 0;
            if (!(fvm_object_property_flags(c, child, "field", &actual) == FVM_OK && actual == flags))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, child, "field", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(out.type == FVM_VALUE_NUMBER && out.number == ((flags & version_masks[version - 4]) ? 11 : 22)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            int own = 0;
            if (!(fvm_object_has_own(c, child, "field", &own) == FVM_OK && own))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* Rotation makes every source bit meet both overlapping and disjoint set/clear masks. */
            const unsigned clear = ((flags << 5) | (flags >> 11)) & 65535;
            const unsigned set = flags ^ 0xa55a;
            const unsigned expected = (flags & ~clear) | set;
            version_flags(c, child, "field", set, clear);
            if (!(fvm_object_property_flags(c, child, "field", &actual) == FVM_OK && actual == expected))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 33);
            if (!(fvm_object_set(c, child, "field", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const unsigned after_write = (expected & 4) ? expected : expected & 7;
            if (!(fvm_object_property_flags(c, child, "field", &actual) == FVM_OK && actual == after_write))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, child, "field", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(out.type == FVM_VALUE_NUMBER &&
                  out.number == ((after_write & version_masks[version - 4]) ? 11 : ((expected & 4) ? 22 : 33))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* Inspect the retained value even when readonly plus version flags hid it. */
            version_flags(c, child, "field", 0, 65535);
            if (!(fvm_object_get(c, child, "field", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(out.type == FVM_VALUE_NUMBER && out.number == ((expected & 4) ? 22 : 33)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            version_flags(c, child, "field", flags, 65535);
            int removed = 0;
            if (!(fvm_object_delete(c, child, "field", &removed) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!!removed == !(flags & 2)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_has_own(c, child, "field", &own) == FVM_OK && !!own == !!(flags & 2)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(flags & 2))
            {
                if (!(fvm_object_property_flags(c, child, "field", &actual) == FVM_OK && !actual))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(c, child, "field", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
                      out.number == 11))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(flags & 4095))
            {
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_owner))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
