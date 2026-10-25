/* UInt16 attributes, counted names and cycles survive graph transport and version replacement. */
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
    for (unsigned source = 4; source <= 8; ++source)
    {
        for (unsigned destination = 0; destination <= 8; ++destination)
        {
            if (destination && destination < 4)
            {
                continue;
            }
            FVM_Context *c = 0, *other = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            version_load(c, source);
            if (destination)
            {
                version_load(other, destination);
            }
            if (!(fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *object = 0;
            FVM_Value value, out, args[4];
            if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = class_value(object);
            if (!(fvm_object_set_bytes(c, object, "K\0Q", 3, &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            args[0] = value;
            fvm_value_string_bytes(args + 1, "k\0q", 3);
            fvm_value_number(args + 2, 0x1000);
            fvm_value_number(args + 3, 0);
            if (!(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned char *data = 0;
            size_t bytes = 0;
            if (!(fvm_value_serialize(c, &value, &data, &bytes) == FVM_OK &&
                  fvm_value_deserialize(other, data, bytes, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_serialized_free(data);
            FVM_Object *copy = out.object;
            if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned flags = 0;
            if (!(fvm_object_property_flags_bytes(other, copy, "K\0Q", 3, &flags) == FVM_OK && flags == 0x1000))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get_bytes(other, copy, "K\0Q", 3, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (destination && destination < 8)
            {
                if (!(out.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(out.type == FVM_VALUE_OBJECT && out.object == copy))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            version_load(other, 8);
            if (!(fvm_object_get_bytes(other, copy, "K\0Q", 3, &out) == FVM_OK && out.object == copy))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            version_load(other, 5);
            if (!(fvm_object_get_bytes(other, copy, "K\0Q", 3, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* A writable hidden field becomes visible after replacement and drops version bits. */
            fvm_value_number(&out, 42);
            if (!(fvm_object_set_bytes(other, copy, "K\0Q", 3, &out) == FVM_OK &&
                  fvm_object_property_flags_bytes(other, copy, "K\0Q", 3, &flags) == FVM_OK && !flags))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get_bytes(other, copy, "K\0Q", 3, &out) == FVM_OK && out.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(other);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
