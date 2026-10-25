/* Real native ToNumber agrees with interpreted Flash-only whitespace coercion. */
#define FVM_NUMERIC_WHITESPACE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_numeric_whitespace.cpp"

#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned short rejected[] = {0, 11, 12, 0xa0, 0x1680, 0x2009, 0x2028, 0x3000, 0xfeff, 0xd800};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (class_mode = 0; class_mode < 3; ++class_mode)
        {
            FVM_Context *c = 0;
            FVM_Value input, out;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            parsing_load(c, version);
            for (unsigned i = 0; i < sizeof(rejected) / sizeof(rejected[0]); ++i)
            {
                unsigned short units[] = {rejected[i], '4', '2'};
                if (!(fvm_value_string_utf16(c, units, 3, &input) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_set_global(c, "whitespaceInput", &input) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program p;
                p.variable("whitespaceInput");
                p.op(0x4a);
                p.finish();
                out = class_run(c, p);
                if (!(out.type == FVM_VALUE_NUMBER))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(version == 4 ? out.number == 0 : out.number != out.number))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program call;
                call.variable("whitespaceInput");
                call.number(1);
                call.text("parseFloat");
                call.op(0x3d);
                call.finish();
                out = class_run(c, call);
                if (!(out.type == FVM_VALUE_NUMBER && out.number != out.number))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
