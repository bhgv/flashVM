/* Retained Number length accepts all IEEE values but rejects invalid metatable graph references. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"
#include "zlib.h"
#include <limits>


static void numeric_wire_u32(unsigned char *out, unsigned long value)
{
    for (unsigned i = 0; i < 4; ++i)
    {
        out[i] = (unsigned char)(value >> (24 - 8 * i));
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
    FVM_Context *source = 0;
    if (!(fvm_context_create(rt, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *array = 0;
    if (!(fvm_object_create(source, &array) == FVM_OK && fvm_gc_pin(source, array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    array->resource_kind = FVM_RESOURCE_ARRAY;
    array->resource_result_valid = true;
    const double inputs[] = {-0.0,
                             -3.9,
                             -2147483649.0,
                             4294967297.9,
                             std::numeric_limits<double>::max(),
                             std::numeric_limits<double>::infinity(),
                             -std::numeric_limits<double>::infinity(),
                             std::numeric_limits<double>::quiet_NaN()};
    for (unsigned i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i)
    {
        fvm_value_number(&array->resource_result, inputs[i]);
        array->resource_result.metatable = array;
        FVM_Value root = class_value(array), out;
        unsigned char *data = 0;
        size_t bytes = 0;
        if (!(fvm_value_serialize(source, &root, &data, &bytes) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(bytes > 46 && data[28] == 1 && data[33] == FVM_VALUE_NUMBER))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Context *other = 0;
        if (!(fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_deserialize(other, data, bytes, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(out.object->resource_result.metatable == out.object &&
              !std::memcmp(&out.object->resource_result.number, inputs + i, sizeof(double))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(other, out.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(other);
        /* One-record graph: reference 1 is outside the graph, despite valid packet CRC. */
        numeric_wire_u32(data + 42, 1);
        numeric_wire_u32(data + 12, (unsigned long)crc32(0, data + 16, (uInt)(bytes - 16)));
        if (!(fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned long before = 0, after = 0;
        if (!(fvm_gc_object_count(other, &before) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_deserialize(other, data, bytes, &out) == FVM_INVALID_ARGUMENT &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_object_count(other, &after) == FVM_OK && before == after))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(other);
        fvm_serialized_free(data);
    }
    fvm_context_destroy(source);
    fvm_runtime_destroy(rt);
}
