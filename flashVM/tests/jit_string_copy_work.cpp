/* String builders, ranges and casing agree across interpreted and native CallMethod. */
#define FVM_STRING_WORK_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "string_loaded_work.cpp"

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
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (class_mode = 0; class_mode < 3; ++class_mode)
        {
            FVM_Context *c = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            string_work_load(c, version);
            const char *text = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
            FVM_Object *str = string_work_make(c, text, 64);
            FVM_Value value = object(str), out;
            if (!(fvm_set_global(c, "textInput", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const char *methods[] = {
                "concat", "slice", "substring", "substr", "toLowerCase", "toUpperCase", "fromCharCode"};
            for (unsigned m = 0; m < 7; ++m)
            {
                Program p;
                if (m == 0)
                {
                    p.text("B");
                    p.number(1);
                }
                else if (m < 4)
                {
                    p.number(64);
                    p.number(0);
                    p.number(2);
                }
                else if (m == 6)
                {
                    p.number(65);
                    p.number(65);
                    p.number(65);
                    p.number(3);
                }
                else
                {
                    p.number(0);
                }
                p.variable(m == 6 ? "String" : "textInput");
                p.text(methods[m]);
                p.op(0x52);
                p.finish();
                out = class_run(c, p);
                if (!(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == (m == 0 ? 65 : m == 6 ? 3 : 64)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (m == 0)
                {
                    if (!(out.string[64] == 'B'))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (m == 4)
                {
                    if (!(out.string[0] == 'a'))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(out.string[0] == 'A'))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                unsigned long old = rt->config.instruction_limit;
                rt->config.instruction_limit = 8;
                class_run(c, p, FVM_ERROR);
                rt->config.instruction_limit = old;
                out = class_run(c, p);
                if (!(out.type == FVM_VALUE_STRING))
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
    }
    fvm_runtime_destroy(rt);
}
