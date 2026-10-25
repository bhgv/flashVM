/* Missing binary values are undefined below the existing top value; raw expectations are independent. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
#include <limits>
#include <cstdio>


static FVM_Value run_missing(FVM_Context *c, unsigned opcode, bool one)
{
    Bytes code;
    if (one)
    {
        integer(code, 7);
    }
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
    return out;
}


static void check(FVM_Value out, unsigned op, unsigned version, bool one)
{
    bool old = version < 7;
    if (!old && (op == 0x48 || op == 0x67))
    {
        if (!(out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return;
    }
    if (op == 0x21)
    {
        if (!(out.type == FVM_VALUE_STRING &&
              std::strcmp(out.string, old ? (one ? "7" : "") : (one ? "undefined7" : "undefinedundefined")) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return;
    }
    if (op == 0x0d && !one && version < 5)
    {
        if (!(out.type == FVM_VALUE_STRING && std::strcmp(out.string, "#ERROR#") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return;
    }
    bool boolean = op == 0x0e || op == 0x0f || op == 0x10 || op == 0x11 || op == 0x13 || op == 0x29 || op == 0x48 ||
                   op == 0x49 || op == 0x66 || op == 0x67 || op == 0x68;
    if (boolean)
    {
        bool expected = false;
        if (op == 0x0e)
        {
            expected = !one && old;
        }
        if (op == 0x0f || op == 0x48 || op == 0x29)
        {
            expected = one && old;
        }
        if (op == 0x11)
        {
            expected = one;
        }
        if (op == 0x13 || op == 0x49 || op == 0x66)
        {
            expected = !one;
        }
        if (op == 0x68)
        {
            expected = one && !old;
        }
        if (out.type != FVM_VALUE_BOOLEAN || !!out.boolean != expected)
        {
            std::fprintf(stderr,
                         "op=%02x version=%u one=%u type=%d bool=%d expected=%d\n",
                         op,
                         version,
                         one,
                         (int)out.type,
                         out.boolean,
                         expected);
        }
        if (!(out.type == FVM_VALUE_BOOLEAN && !!out.boolean == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return;
    }
    if (!(out.type == FVM_VALUE_NUMBER))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (op >= 0x60 && op <= 0x65)
    {
        if (!(out.number == ((one && (op == 0x61 || op == 0x62)) ? 7 : 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return;
    }
    if ((!old && (op == 0x0a || op == 0x0b || op == 0x0c || op == 0x0d || op == 0x47)) ||
        (op == 0x3f && (!one || !old)) || (op == 0x0d && !one))
    {
        if (!(out.number != out.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (one && (op == 0x0a || op == 0x47))
    {
        if (!(out.number == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (one && op == 0x0b)
    {
        if (!(out.number == -7))
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


int main()
{
    const unsigned ops[] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x13, 0x21, 0x29, 0x3f,
                            0x47, 0x48, 0x49, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68};
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
            for (unsigned one = 0; one < 2; ++one)
            {
                if (mode)
                {
                    check(run_missing(c, ops[i], one != 0), ops[i], version, one != 0);
                }
                else
                {
                    Bytes code;
                    if (one)
                    {
                        integer(code, 7);
                    }
                    code.push_back((unsigned char)ops[i]);
                    code.push_back(0);
                    if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_ERROR))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        if (mode)
        {
            FVM_Object *obj = 0;
            if (!(fvm_object_create(c, &obj) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned id = 2;
            attach(c, obj, "valueOf", numeric, &id);
            FVM_Value value;
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = obj;
            if (!(fvm_set_global(c, "right", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Bytes code;
            text(code, "right");
            code.push_back(0x1c);
            code.push_back(0x0b);
            text(code, "result");
            code.push_back(0x4d);
            code.push_back(0x1d);
            code.push_back(0);
            calls = 0;
            if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK && calls == 1 && order[0] == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_get_global(c, "result", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version < 7)
            {
                if (!(value.number == -2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(value.number != value.number))
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
