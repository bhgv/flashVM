/* Loaded decimal parsing retains AVM1 digit accumulation, dots and exponent wrap. */
#define FVM_PARSE_INT_LOADED_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "global_parse_int_loaded.cpp"


static void check_float(FVM_Context *c, const char *text, size_t length, double expected)
{
    FVM_Value input, out;
    if (!(fvm_value_string_bytes(&input, text, length) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_call(c, "parseFloat", 1, &input, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (expected != expected)
    {
        if (!(out.number != out.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (expected == 0)
    {
        if (!(out.number == 0 && (1.0 / out.number < 0) == (1.0 / expected < 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (std::fabs(expected) == std::numeric_limits<double>::infinity())
    {
        if (!(out.number == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(std::fabs(out.number - expected) <= std::fabs(expected) * 1e-14))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
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
    const double nan = std::numeric_limits<double>::quiet_NaN();


    struct Case
    {
        const char *text;
        double expected;
    };

    const Case cases[] = {{"Infinity", nan},
                          {"-Infinity", nan},
                          {"+Infinity", nan},
                          {"", nan},
                          {"-", nan},
                          {".", 0},
                          {"-.", -0.0},
                          {"..1", .1},
                          {"1.2345.678", 1.2345678},
                          {"1.2345.6e50", 1.23456},
                          {"1e4294967297", 10},
                          {"1e-4294967297", .1},
                          {"1e2147483648", 0},
                          {"9e99999", std::numeric_limits<double>::infinity()},
                          {"10e", 10},
                          {"10e-", 10},
                          {"0xff", 0},
                          {" .125e+2tail", 12.5},
                          {"\302\240\357\273\27742.5", nan},
                          {"\355\240\20012", nan}};
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        FVM_Value out, input;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        parsing_load(c, version);
        if (!(fvm_call(c, "parseFloat", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        {
            check_float(c, cases[i].text, std::strlen(cases[i].text), cases[i].expected);
        }
        check_float(c, "1.5\0.2", 6, 1.5);
        const char raw[] = {'1', '.', '2', '5'};
        check_float(c, raw, sizeof(raw), 1.25);
        fvm_value_string(&input, ".1499999");
        if (!(fvm_call(c, "parseFloat", 1, &input, &out) == FVM_OK && out.number == 0.14999990000000005))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_boolean(&input, 1);
        if (!(fvm_call(c, "parseFloat", 1, &input, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(version == 4 ? out.number == 1 : out.number != out.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *obj = 0;
        if (!(fvm_object_create(c, &obj) == FVM_OK && fvm_gc_pin(c, obj) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, obj, "toString", parsing_text, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&input);
        input.type = FVM_VALUE_OBJECT;
        input.object = obj;
        for (unsigned i = 0; i < 3; ++i)
        {
            parsing_mode = i == 0 ? 0 : i == 1 ? 3 : 4;
            parsing_order = 0;
            FVM_Result status = fvm_call(c, "parseFloat", 1, &input, &out);
            if (!(parsing_order == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (i == 1)
            {
                if (!(status == FVM_EXCEPTION))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_exception_clear(c) == FVM_OK))
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
                if (!(i == 0 ? out.number == 0 : out.number != out.number))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->gc_frames && !c->gc_native_depth && !c->primitive_depth && !c->string_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
