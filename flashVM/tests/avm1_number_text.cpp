#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include "flashvm/execution.h"
#include <cstring>
#include <limits>


struct Case
{
    double number;
    const char *text;
};

static const Case cases[] = {{0, "0"},
                             {-0.0, "0"},
                             {1, "1"},
                             {5, "5"},
                             {1.4, "1.4"},
                             {-990.123, "-990.123"},
                             {9.9999e14, "999990000000000"},
                             {-9.9999e14, "-999990000000000"},
                             {1e15, "1e+15"},
                             {-1e15, "-1e+15"},
                             {1e-5, "0.00001"},
                             {-1e-5, "-0.00001"},
                             {0.999e-5, "9.99e-6"},
                             {-0.999e-5, "-9.99e-6"},
                             {0.19999999999999996, "0.2"},
                             {-0.19999999999999996, "-0.2"},
                             {100000.12345678912, "100000.123456789"},
                             {-100000.12345678912, "-100000.123456789"},
                             {0.8000000000000005, "0.800000000000001"},
                             {-0.8000000000000005, "-0.800000000000001"},
                             {0.8300000000000005, "0.83"},
                             {1e-320, "9.99988867182684e-321"},
                             {std::numeric_limits<double>::min(), "2.2250738585072e-308"},
                             {std::numeric_limits<double>::max(), "1.79769313486231e+308"},
                             {-std::numeric_limits<double>::max(), "-1.79769313486231e+308"},
                             {5e-324, "4.94065645841247e-324"},
                             {9.999999999999999, "10"},
                             {-9.999999999999999, "-10"},
                             {9999999999999996.0, "1e+16"},
                             {-9999999999999996.0, "-e+16"},
                             {0.000009999999999999996, "1e-5"},
                             {-0.000009999999999999996, "-10e-6"},
                             {0.00009999999999999996, "0.0001"},
                             {-0.00009999999999999996, "-0.0001"},
                             {std::numeric_limits<double>::quiet_NaN(), "NaN"},
                             {std::numeric_limits<double>::infinity(), "Infinity"},
                             {-std::numeric_limits<double>::infinity(), "-Infinity"}};


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
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK &&
              fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        {
            FVM_Value value, out;
            fvm_value_number(&value, cases[i].number);
            if (!(fvm_value_to_string(c, &value, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, cases[i].text)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_GCRootScope *roots = 0;
            if (!(fvm_gc_roots_begin(c, &roots) == FVM_OK && fvm_gc_roots_add(roots, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !std::strcmp(out.string, cases[i].text)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_roots_end(roots) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_Value a, b, out;
        fvm_value_number(&a, std::numeric_limits<double>::quiet_NaN());
        b = a;
        if (!(fvm_exec_compare(c, FVM_EXEC_STRICT_EQUAL, &a, &b, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&b, "NaN");
        if (!(fvm_exec_compare(c, FVM_EXEC_STRICT_EQUAL, &a, &b, &out) == FVM_OK && !out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    FVM_Context *host = 0;
    if (!(fvm_context_create(rt, &host) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value a, b, out;
    fvm_value_number(&a, std::numeric_limits<double>::quiet_NaN());
    b = a;
    if (!(fvm_exec_compare(host, FVM_EXEC_STRICT_EQUAL, &a, &b, &out) == FVM_OK && !out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&a, 1e15);
    if (!(fvm_value_to_string(host, &a, &out) == FVM_OK && !std::strcmp(out.string, "1000000000000000")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(host);
    fvm_runtime_destroy(rt);
    return 0;
}
