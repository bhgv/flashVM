/* Canonical case selection must not publish partial attribute maps on allocation failure. */
#define main native_instance_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "../src/core/property_key.hpp"
#include <cstring>


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char stored[] = "VeryLongPropertyNameForCanonicalAttributeSpellingAndAllocationChecks";
    const char query[] = "verylongpropertynameforcanonicalattributespellingandallocationchecks";
    const char nul[] = "MiXeD\0\xc3\x84", lower[] = "mixed\0\xc3\xa4";
    for (unsigned version = 4; version <= 8; ++version)
    {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 1000 && !success; ++boundary)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *object = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value args[4], value, out;
            fvm_value_number(&value, 9);
            if (!(fvm_object_set(c, object, stored, &value) == FVM_OK &&
                  fvm_object_set_bytes(c, object, nul, sizeof(nul) - 1, &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_set_property_flags(c, object, stored, 1, 0) == FVM_OK &&
                  fvm_object_set_property_flags_bytes(c, object, nul, sizeof(nul) - 1, 4, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_detail::vector<char> names;
            for (size_t i = 0; i < sizeof(query) - 1; ++i)
            {
                names.push_back(query[i]);
            }
            names.push_back(',');
            for (size_t i = 0; i < sizeof(lower) - 1; ++i)
            {
                names.push_back(lower[i]);
            }
            names.push_back(',');
            for (size_t i = 0; i < sizeof(query) - 1; ++i)
            {
                names.push_back(query[i]);
            }
            fvm_value_undefined(args);
            args[0].type = FVM_VALUE_OBJECT;
            args[0].object = object;
            if (!(fvm_value_string_bytes(args + 1, names.begin(), names.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(args + 2, 2);
            fvm_value_number(args + 3, 1);
            allowance = boundary;
            FVM_Result status = fvm_call(c, "ASSetPropFlags", 4, args, &out);
            allowance = -1;
            if (!(!c->gc_frames && !c->gc_native_depth && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned actual = 99;
            if (status == FVM_OUT_OF_MEMORY)
            {
                failed = true;
                if (!(fvm_object_property_flags(c, object, stored, &actual) == FVM_OK && actual == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_property_flags_bytes(c, object, nul, sizeof(nul) - 1, &actual) == FVM_OK &&
                      actual == 4))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(status == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                success = true;
            }
            if (!(fvm_object_property_flags(c, object, stored, &actual) == FVM_OK && actual == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_property_flags_bytes(c, object, nul, sizeof(nul) - 1, &actual) == FVM_OK && actual == 6))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_detail::string query_key;
            if (!(fvm_property_key(lower, sizeof(lower) - 1, query_key) == FVM_OK &&
                  object->attributes.find(query_key) == object->attributes.end() &&
                  object->attributes.find(query) == object->attributes.end()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
        if (!(failed && success))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_runtime_destroy(rt);
}
