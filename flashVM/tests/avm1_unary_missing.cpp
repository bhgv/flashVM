/* Missing unary operands become undefined only for loaded SWFs; raw oracle values stay independent. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"


static void check(FVM_Context *c, unsigned opcode, unsigned version)
{
    Bytes code;
    code.push_back((unsigned char)opcode);
    text(code, "result");
    code.push_back(0x4d);
    code.push_back(0x1d);
    code.push_back(0);
    if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value out;
    if (!(fvm_get_global(c, "result", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (opcode == 0x12)
    {
        if (!(out.type == FVM_VALUE_BOOLEAN && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (opcode == 0x44)
    {
        if (!(out.type == FVM_VALUE_STRING && std::strcmp(out.string, "undefined") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (opcode == 0x4b)
    {
        if (!(out.type == FVM_VALUE_STRING && std::strcmp(out.string, version < 7 ? "" : "undefined") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (opcode == 0x33 || opcode == 0x37)
    {
        if (!(out.type == FVM_VALUE_STRING && std::strcmp(out.string, "") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(out.type == FVM_VALUE_NUMBER))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if ((opcode == 0x4a || opcode == 0x50 || opcode == 0x51) && version >= 7)
        {
            if (!(out.number != out.number))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (opcode == 0x50)
        {
            if (!(out.number == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (opcode == 0x51)
        {
            if (!(out.number == -1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (opcode == 0x14 || opcode == 0x31)
        {
            if (!(out.number == (version < 7 ? 0 : 9)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (opcode == 0x32 || opcode == 0x36)
        {
            if (!(out.number == (version < 7 ? 0 : 117)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(out.number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
}


int main()
{
    const unsigned ops[] = {0x12, 0x14, 0x18, 0x31, 0x32, 0x33, 0x36, 0x37, 0x44, 0x4a, 0x4b, 0x50, 0x51};
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 6; ++mode)
    {
        FVM_Context *c = 0;
        unsigned version = mode + 3;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (mode)
        {
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < sizeof(ops) / sizeof(ops[0]); ++i)
        {
            if (mode)
            {
                check(c, ops[i], version);
            }
            else
            {
                unsigned char code[2] = {(unsigned char)ops[i], 0};
                if (!(fvm_avm1_execute(c, code, sizeof(code)) == FVM_ERROR))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
