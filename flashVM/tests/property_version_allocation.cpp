/* New/new[] failures preserve staged UInt16 flags and graph decode remains transactional. */
#define main native_instance_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "property_version_fixture.hpp"
#include "zlib.h"


static void version_wire_u32(unsigned char *p, unsigned long value)
{
    p[0] = (unsigned char)(value >> 24);
    p[1] = (unsigned char)(value >> 16);
    p[2] = (unsigned char)(value >> 8);
    p[3] = (unsigned char)value;
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
            version_load(c, version);
            if (!(fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *object = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value value, args[4], out;
            value = class_value(object);
            if (!(fvm_object_set(c, object, "field", &value) == FVM_OK &&
                  fvm_set_global(c, "versionObject", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            args[0] = value;
            fvm_value_string(args + 1, "field");
            fvm_value_number(args + 2, 65535);
            fvm_value_number(args + 3, 0);
            allowance = boundary;
            FVM_Result status = fvm_call(c, "ASSetPropFlags", 4, args, &out);
            allowance = -1;
            unsigned flags = 99;
            if (status == FVM_OUT_OF_MEMORY)
            {
                failed = true;
                if (!(fvm_object_property_flags(c, object, "field", &flags) == FVM_OK && !flags &&
                      fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK))
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
            if (!(fvm_object_property_flags(c, object, "field", &flags) == FVM_OK && flags == 65535 && !c->gc_frames &&
                  !c->gc_native_depth))
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
    FVM_Context *source = 0;
    if (!(fvm_context_create(rt, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    version_load(source, 8);
    if (!(fvm_standard_install(source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *object = 0;
    FVM_Value value, copy;
    if (!(fvm_object_create(source, &object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = class_value(object);
    if (!(fvm_object_set(source, object, "field", &value) == FVM_OK && fvm_gc_pin(source, object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    version_flags(source, object, "field", 65535, 0);
    unsigned char *data = 0;
    size_t bytes = 0;
    if (!(fvm_value_serialize(source, &value, &data, &bytes) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool failed = false, success = false;
    for (long boundary = 0; boundary < 1000 && !success; ++boundary)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        version_load(c, 8);
        unsigned long before = 0, after = 0;
        if (!(fvm_gc_object_count(c, &before) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = boundary;
        FVM_Result status = fvm_value_deserialize(c, data, bytes, &copy);
        allowance = -1;
        if (status == FVM_OUT_OF_MEMORY)
        {
            failed = true;
            if (!(copy.type == FVM_VALUE_UNDEFINED && fvm_gc_object_count(c, &after) == FVM_OK && before == after &&
                  fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_value_deserialize(c, data, bytes, &copy) == FVM_OK))
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
        unsigned flags = 0;
        if (!(fvm_object_property_flags(c, copy.object, "field", &flags) == FVM_OK && flags == 65535 && !c->gc_frames &&
              !c->gc_native_depth))
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
    /* A CRC-valid attribute outside UInt16 must fail before destination allocation. */
    size_t offset = 0;
    for (size_t i = 0; i + 9 <= bytes; ++i)
    {
        if (!std::memcmp(data + i, "field", 5) && !data[i + 5] && !data[i + 6] && data[i + 7] == 255 &&
            data[i + 8] == 255)
        {
            offset = i + 5;
        }
    }
    if (!(offset))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    version_wire_u32(data + offset, 65536);
    version_wire_u32(data + 12, (unsigned long)crc32(0, data + 16, (uInt)(bytes - 16)));
    FVM_Context *bad = 0;
    unsigned long before = 0, after = 0;
    if (!(fvm_context_create(rt, &bad) == FVM_OK && fvm_gc_object_count(bad, &before) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_deserialize(bad, data, bytes, &copy) == FVM_INVALID_ARGUMENT && copy.type == FVM_VALUE_UNDEFINED &&
          fvm_gc_object_count(bad, &after) == FVM_OK && before == after))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(bad);
    object->attributes["field"] = 65536;
    unsigned char *invalid = 0;
    size_t invalid_bytes = 0;
    if (!(fvm_value_serialize(source, &value, &invalid, &invalid_bytes) == FVM_INVALID_ARGUMENT && !invalid &&
          !invalid_bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(data);
    fvm_context_destroy(source);
    fvm_runtime_destroy(rt);
}
