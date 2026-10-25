/* Loaded String range copies share native work without publishing partial text. */
#define FVM_STRING_WORK_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "string_loaded_work.cpp"


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
        FVM_Object *str = string_work_make(c,
                                           "ab\xf0\x9f\x98\x80"
                                           "cd",
                                           8);
        const char *names[] = {"slice", "substring", "substr"};
        for (unsigned method = 0; method < 3; ++method)
        {
            FVM_Value args[2], out;
            fvm_value_number(args, 2);
            fvm_value_number(args + 1, method == 2 ? 2 : 4);
            rt->config.instruction_limit = 1;
            fvm_value_number(&out, 77);
            FVM_Result result = fvm_object_call(c, str, names[method], 2, args, &out);
            if (!(result == (version ? FVM_ERROR : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                if (!(out.type == FVM_VALUE_UNDEFINED && c->avm1_steps == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            rt->config.instruction_limit = 2;
            if (!(fvm_object_call(c, str, names[method], 2, args, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 4 &&
                  !std::memcmp(out.string, "\xf0\x9f\x98\x80", 4)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                if (!(c->avm1_steps == 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                c->avm1_execution_depth = 1;
                c->avm1_steps = 1;
                if (!(fvm_object_call(c, str, names[method], 2, args, &out) == FVM_ERROR && c->avm1_steps == 2 &&
                      c->avm1_execution_depth == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                rt->config.instruction_limit = 0;
                c->avm1_steps = std::numeric_limits<unsigned long>::max();
                if (!(fvm_object_call(c, str, names[method], 2, args, &out) == FVM_ERROR &&
                      c->avm1_steps == std::numeric_limits<unsigned long>::max()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                c->avm1_execution_depth = 0;
                /* Missing-argument noops do not reset or consume an existing budget. */
                c->avm1_steps = 17;
                if (!(fvm_object_call(c, str, names[method], 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED &&
                      c->avm1_steps == 17))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            rt->config.instruction_limit = 0;
            fvm_value_number(args, 3);
            fvm_value_number(args + 1, method == 2 ? 1 : 4);
            if (!(fvm_object_call(c, str, names[method], 2, args, &out) == FVM_OK && fvm_value_string_size(&out) == 3 &&
                  !std::memcmp(out.string, "\xed\xb8\x80", 3)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            string_work_clean(c);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
