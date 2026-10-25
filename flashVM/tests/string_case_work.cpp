/* Public loaded casing charges UCS2 mapping and UTF-16 output independently. */
#define FVM_STRING_WORK_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "string_loaded_work.cpp"
#include "../src/standard/string_state.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        string_work_load(c, version);
        const char input[] = {'A', 0, (char)0xed, (char)0xa0, (char)0x80, 'b'};
        FVM_Object *str = string_work_make(c, input, sizeof(input));
        FVM_Value out;
        const char *names[] = {"toLowerCase", "toUpperCase"};
        for (unsigned m = 0; m < 2; ++m)
        {
            rt->config.instruction_limit = 7;
            if (!(fvm_object_call(c, str, names[m], 0, 0, &out) == (version ? FVM_ERROR : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                if (!(out.type == FVM_VALUE_UNDEFINED && c->avm1_steps == 7))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            rt->config.instruction_limit = 8;
            if (!(fvm_object_call(c, str, names[m], 0, 0, &out) == FVM_OK &&
                  fvm_value_string_size(&out) == sizeof(input)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(out.string[0] == (m ? 'A' : 'a') && out.string[1] == 0 &&
                  !std::memcmp(out.string + 2, input + 2, 3) && out.string[5] == (m ? 'B' : 'b')))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                if (!(c->avm1_steps == 8))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                c->avm1_execution_depth = 1;
                c->avm1_steps = 7;
                if (!(fvm_object_call(c, str, names[m], 0, 0, &out) == FVM_ERROR && c->avm1_steps == 8 &&
                      c->avm1_execution_depth == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                rt->config.instruction_limit = 0;
                c->avm1_steps = std::numeric_limits<unsigned long>::max();
                if (!(fvm_object_call(c, str, names[m], 0, 0, &out) == FVM_ERROR &&
                      c->avm1_steps == std::numeric_limits<unsigned long>::max()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                c->avm1_execution_depth = 0;
                /* Internal property folding retains its caller's existing accounting policy. */
                FVM_Value text;
                if (!(fvm_value_string_bytes(&text, input, sizeof(input)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                c->avm1_steps = 17;
                if (!(fvm_string_case(c, text, &out, m != 0) == FVM_OK && c->avm1_steps == 17))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            rt->config.instruction_limit = 0;
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            string_work_clean(c);
        }
        if (version)
        {
            FVM_Object *pair = string_work_make(c, "\xf0\x9f\x98\x80", 4);
            rt->config.instruction_limit = 3;
            if (!(fvm_object_call(c, pair, "toLowerCase", 0, 0, &out) == FVM_ERROR && c->avm1_steps == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            rt->config.instruction_limit = 4;
            if (!(fvm_object_call(c, pair, "toLowerCase", 0, 0, &out) == FVM_OK && fvm_value_string_size(&out) == 4 &&
                  c->avm1_steps == 4))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        rt->config.instruction_limit = 0;
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
