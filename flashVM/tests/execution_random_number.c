/* C89 bounded random values share the existing deterministic Math sequence. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/serialization.h"
#include <float.h>
#include <math.h>


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned version;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (version = 4; version <= 8; ++version)
    {
        FVM_Context *c;
        FVM_Value input, out, math;
        unsigned i;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (i = 0; i < 6; ++i)
        {
            fvm_value_undefined(&input);
            if (i == 1)
            {
                input.type = FVM_VALUE_NULL;
            }
            if (i == 2)
            {
                fvm_value_number(&input, -17);
            }
            if (i == 3)
            {
                fvm_value_number(&input, 0.9);
            }
            if (i == 4)
            {
                if (!(fvm_value_string(&input, "invalid-number") == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (i == 5)
            {
                fvm_value_number(&input, -HUGE_VAL);
            }
            if (!(fvm_exec_random_number(c, &input, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_number(&input, 10);
        if (!(fvm_exec_random_number(c, &input, &input) == FVM_OK && input.number == 8))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "Math", &math) == FVM_OK && math.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, math.object, "random", 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(out.number == 1586005467.0 / 4294967296.0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&input, 4294967297.0);
        if (!(fvm_exec_random_number(c, &input, &out) == FVM_OK && out.number == 18219391))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        {
            FVM_Context *dst;
            FVM_Value restored, other;
            unsigned char *data;
            size_t size;
            if (!(fvm_value_serialize(c, &math, &data, &size) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_context_create(rt, &dst) == FVM_OK && fvm_load_memory(dst, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_standard_install(dst) == FVM_OK && fvm_value_deserialize(dst, data, size, &restored) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&input, 10);
            if (!(fvm_exec_random_number(c, &input, &out) == FVM_OK && out.number == 5))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exec_random_number(dst, &input, &other) == FVM_OK && other.number == out.number))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_serialized_free(data);
            fvm_context_destroy(dst);
        }
        for (i = 0; i < 5; ++i)
        {
            fvm_value_number(&input, DBL_MAX);
            if (i == 1)
            {
                fvm_value_number(&input, HUGE_VAL);
            }
            if (i == 2)
            {
                fvm_value_number(&input, 1.9);
            }
            if (i == 3)
            {
                if (!(fvm_value_string(&input, "17") == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (i == 4)
            {
                fvm_value_boolean(&input, 1);
            }
            if (!(fvm_exec_random_number(c, &input, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(out.number >= 0 && out.number < 2147483647.0 && out.number == floor(out.number)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (i == 2 || i == 4)
            {
                if (!(out.number == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (i == 3)
            {
                if (!(out.number < 17))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        fvm_value_number(&out, 99);
        if (!(fvm_exec_random_number(0, &input, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_random_number(c, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_random_number(c, &input, 0) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
