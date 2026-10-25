/* Native signed splice allocation failures restore roots and permit retry of partial mutation. */
#define main native_splice_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
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
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary)
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
            FVM_Value value, args[4], out;
            fvm_value_number(&value, -3);
            FVM_Object *array = 0;
            if (!(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(args, 0);
            fvm_value_number(args + 1, 0);
            if (!(fvm_value_string_bytes(args + 2, "first\0tail", 10) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_value_string_bytes(args + 3, "second\0tail", 11) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            allowance = boundary;
            FVM_Result status = fvm_object_call(c, array, "splice", 4, args, &out);
            allowance = -1;
            if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->exception_pending))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (status == FVM_OUT_OF_MEMORY)
            {
                failed = true;
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_call(c, array, "splice", 4, args, &out) == FVM_OK))
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
                if (!(out.type == FVM_VALUE_OBJECT && fvm_object_get(c, out.object, "length", &value) == FVM_OK &&
                      value.number == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(c, array, "length", &value) == FVM_OK && value.number == -1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(c, array, "-3", &value) == FVM_OK && value.type == FVM_VALUE_STRING &&
                      fvm_value_string_size(&value) == 10 && !std::memcmp(value.string, "first\0tail", 10)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(c, array, "-2", &value) == FVM_OK && value.type == FVM_VALUE_STRING &&
                      fvm_value_string_size(&value) == 11 && !std::memcmp(value.string, "second\0tail", 11)))
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
