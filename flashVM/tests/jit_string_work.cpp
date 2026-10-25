/* String traversal uses the same work budget in interpreted and native CallMethod. */
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
            FVM_Object *str = string_work_make(c, "a,b,c", 5);
            FVM_Value value = object(str), out;
            if (!(fvm_set_global(c, "textInput", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const char *methods[] = {"split", "indexOf", "lastIndexOf"};
            for (unsigned m = 0; m < 3; ++m)
            {
                Program p;
                p.text(",");
                p.number(1);
                p.variable("textInput");
                p.text(methods[m]);
                p.op(0x52);
                p.finish();
                out = class_run(c, p);
                if (!m)
                {
                    number(c, out.object, "length", 3);
                }
                else
                {
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == (m == 1 ? 1 : 3)))
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
                if (!m)
                {
                    number(c, out.object, "length", 3);
                }
                else
                {
                    if (!(out.number == (m == 1 ? 1 : 3)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                string_work_clean(c);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
