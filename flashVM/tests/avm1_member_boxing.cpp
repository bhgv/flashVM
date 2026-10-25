/* SetMember on temporary primitive/nullish wrappers must not abort loaded actions. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_standard_types.cpp"


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
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes code;
        for (unsigned kind = 0; kind < 5; ++kind)
        {
            if (kind == 0)
            {
                integer(code, 42);
            }
            else if (kind == 1)
            {
                text(code, "abc");
            }
            else
            {
                code.push_back(0x96);
                code.push_back(kind == 2 ? 2 : 1);
                code.push_back(0);
                code.push_back(kind == 2 ? 5 : kind == 3 ? 2 : 3);
                if (kind == 2)
                {
                    code.push_back(1);
                }
            }
            text(code, "cell");
            integer(code, 17);
            code.push_back(0x4f);
        }
        text(code, "done");
        FVM_Value out = run(c, code);
        if (!(out.type == FVM_VALUE_STRING && !strcmp(out.string, "done")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
