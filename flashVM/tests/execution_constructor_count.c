/* C89 named-constructor counts retain the 65536 host boundary and loaded capping/wrapping. */
#include <stdio.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include <math.h>
#include <stdlib.h>


typedef struct Case
{
    double value;
    size_t available;
    FVM_Result host, loaded;
    unsigned result;
} Case;


int main(void)
{
    Case cases[] = {{65536, 65536, FVM_OK, FVM_ERROR, 65536},
                    {65536, 65535, FVM_ERROR, FVM_OK, 65535},
                    {65537, 65536, FVM_ERROR, FVM_ERROR, 0},
                    {65535, 65535, FVM_OK, FVM_OK, 65535},
                    {2, 1, FVM_ERROR, FVM_OK, 1},
                    {1.9, 7, FVM_ERROR, FVM_OK, 1},
                    {-1, 7, FVM_ERROR, FVM_OK, 7},
                    {4294967298.0, 7, FVM_ERROR, FVM_OK, 2},
                    {0, 7, FVM_ERROR, FVM_OK, 0},
                    {0, 7, FVM_ERROR, FVM_OK, 0},
                    {0, 0, FVM_OK, FVM_OK, 0}};
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned profile, i;
    cases[8].value = strtod("NaN", 0);
    cases[9].value = HUGE_VAL;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c, *other;
        FVM_Object *foreign;
        FVM_Value v;
        unsigned out;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
        {
            swf[3] = (unsigned char)(profile + 3);
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        {
            FVM_Result expected = profile ? cases[i].loaded : cases[i].host;
            fvm_value_number(&v, cases[i].value);
            out = 99;
            if (!(fvm_exec_constructor_count(c, &v, cases[i].available, &out) == expected &&
                  out == (expected == FVM_OK ? cases[i].result : 99)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_string(&v, "1.9");
        out = 99;
        if (!(fvm_exec_constructor_count(c, &v, 7, &out) == (profile ? FVM_OK : FVM_ERROR) &&
              out == (profile ? 1 : 99)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        out = 99;
        if (!(fvm_exec_constructor_count(c, 0, 7, &out) == FVM_INVALID_ARGUMENT && out == 99 &&
              fvm_exec_constructor_count(0, &v, 7, &out) == FVM_INVALID_ARGUMENT && out == 99 &&
              fvm_exec_constructor_count(c, &v, 7, 0) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(other, &foreign) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = foreign;
        if (!(fvm_exec_constructor_count(c, &v, 7, &out) == FVM_INVALID_ARGUMENT && out == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!profile)
        {
            FVM_Value *args = (FVM_Value *)malloc(FVM_EXEC_MAX_CONSTRUCTOR_ARGUMENTS * sizeof(FVM_Value));
            FVM_Value name, result, length;
            unsigned argc = 0;
            if (!(args && fvm_standard_install(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (i = 0; i < FVM_EXEC_MAX_CONSTRUCTOR_ARGUMENTS; ++i)
            {
                fvm_value_number(args + i, 2);
            }
            fvm_value_number(&v, FVM_EXEC_MAX_CONSTRUCTOR_ARGUMENTS);
            fvm_value_string(&name, "Array");
            if (!(fvm_exec_constructor_count(c, &v, FVM_EXEC_MAX_CONSTRUCTOR_ARGUMENTS, &argc) == FVM_OK &&
                  fvm_exec_new_object(c, 0, 0, 0, 0, &name, argc, args, &result) == FVM_OK &&
                  result.type == FVM_VALUE_OBJECT && fvm_object_get(c, result.object, "length", &length) == FVM_OK &&
                  length.number == FVM_EXEC_MAX_CONSTRUCTOR_ARGUMENTS))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            free(args);
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
