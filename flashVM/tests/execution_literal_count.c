/* C89 literal counts distinguish signed rejection, materialization limits and host errors. */
#include <stdio.h>
#include "flashvm/execution.h"
#include <math.h>
#include <stdlib.h>


typedef struct Case
{
    double input;
    FVM_Result host, loaded;
    FVM_ValueType type;
    double normalized;
} Case;


int main(void)
{
    Case cases[] = {{0, FVM_OK, FVM_OK, FVM_VALUE_NUMBER, 0},
                    {1.9, FVM_INVALID_ARGUMENT, FVM_OK, FVM_VALUE_NUMBER, 1},
                    {-0.5, FVM_INVALID_ARGUMENT, FVM_OK, FVM_VALUE_UNDEFINED, 0},
                    {0, FVM_INVALID_ARGUMENT, FVM_OK, FVM_VALUE_NUMBER, 0},
                    {0, FVM_INVALID_ARGUMENT, FVM_OK, FVM_VALUE_UNDEFINED, 0},
                    {0, FVM_INVALID_ARGUMENT, FVM_OK, FVM_VALUE_UNDEFINED, 0},
                    {65536, FVM_OK, FVM_OK, FVM_VALUE_NUMBER, 65536},
                    {65536.9, FVM_INVALID_ARGUMENT, FVM_OK, FVM_VALUE_NUMBER, 65536},
                    {65537, FVM_INVALID_ARGUMENT, FVM_UNSUPPORTED, FVM_VALUE_UNDEFINED, 0},
                    {2147483647.0, FVM_INVALID_ARGUMENT, FVM_UNSUPPORTED, FVM_VALUE_UNDEFINED, 0},
                    {2147483648.0, FVM_INVALID_ARGUMENT, FVM_OK, FVM_VALUE_UNDEFINED, 0}};
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned profile, i;
    cases[3].input = strtod("NaN", 0);
    cases[4].input = HUGE_VAL;
    cases[5].input = -HUGE_VAL;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c;
        FVM_Value v, out;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_context_create(rt, &c) == FVM_OK))
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
            FVM_Result status = profile ? cases[i].loaded : cases[i].host;
            fvm_value_number(&v, cases[i].input);
            fvm_value_number(&out, 99);
            if (!(fvm_exec_literal_count(c, &v, &out) == status))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (status != FVM_OK)
            {
                if (!(out.type == FVM_VALUE_NUMBER && out.number == 99))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else if (profile)
            {
                if (!(out.type == cases[i].type &&
                      (out.type == FVM_VALUE_UNDEFINED || out.number == cases[i].normalized)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(out.type == FVM_VALUE_NUMBER && out.number == cases[i].input))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        fvm_value_string(&v, "1.9");
        if (!(fvm_exec_literal_count(c, &v, &v) == (profile ? FVM_OK : FVM_ERROR)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
        {
            if (!(v.type == FVM_VALUE_NUMBER && v.number == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(v.type == FVM_VALUE_STRING))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_undefined(&v);
        if (!(fvm_exec_literal_count(c, &v, &out) == (profile ? FVM_OK : FVM_ERROR)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
        {
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_number(&out, 99);
        if (!(fvm_exec_literal_count(c, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 99 &&
              fvm_exec_literal_count(0, &v, &out) == FVM_INVALID_ARGUMENT && out.number == 99 &&
              fvm_exec_literal_count(c, &v, 0) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
