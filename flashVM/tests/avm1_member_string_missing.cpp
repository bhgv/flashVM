/* String/member operands are filled below existing values; name callbacks retain roots. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"


static void finish(Bytes &code)
{
    text(code, "result");
    code.push_back(0x4d);
    code.push_back(0x1d);
    code.push_back(0);
}


static FVM_Value run(FVM_Context *c, Bytes code)
{
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

static unsigned converted;


static FVM_Result key(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++converted;
    int deleted = 0;
    if (!(fvm_object_delete(c, c->global, "name", &deleted) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, c->global, "assigned", &deleted) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_value_string(out, "victim");
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
        for (unsigned mb = 0; mb < 2; ++mb)
        {
            for (unsigned count = 0; count < 3; ++count)
            {
                Bytes code;
                if (count == 2)
                {
                    integer(code, 2);
                    integer(code, 3);
                }
                else if (count == 1)
                {
                    integer(code, 7);
                }
                code.push_back(mb ? 0x35 : 0x15);
                finish(code);
                if (!mode)
                {
                    if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_ERROR))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    FVM_Value out = run(c, code);
                    if (!(out.type == FVM_VALUE_STRING && std::strcmp(out.string,
                                                                      version < 7 || !count ? ""
                                                                      : count == 1          ? "undefin"
                                                                                            : "nde") == 0))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        const unsigned ops[] = {0x4e, 0x4f, 0x3a};
        for (unsigned i = 0; i < 3; ++i)
        {
            for (unsigned count = 0; count < (i == 1 ? 3U : 2U); ++count)
            {
                Bytes code;
                if (count == 2)
                {
                    text(code, "key");
                }
                if (count)
                {
                    integer(code, 7);
                }
                code.push_back((unsigned char)ops[i]);
                if (i == 1)
                {
                    text(code, "result");
                    integer(code, 123);
                    code.push_back(0x1d);
                    code.push_back(0);
                }
                else
                {
                    finish(code);
                }
                if (!mode)
                {
                    if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_ERROR))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    FVM_Value out = run(c, code);
                    if (i == 0)
                    {
                        if (!(out.type == FVM_VALUE_UNDEFINED))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (i == 1)
                    {
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == 123))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(out.type == FVM_VALUE_BOOLEAN && !out.boolean))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
            }
        }
        if (mode)
        {
            for (unsigned i = 0; i < 3; ++i)
            {
                FVM_Object *name = 0, *assigned = 0;
                if (!(fvm_object_create(c, &name) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                attach(c, name, "toString", key, 0);
                FVM_Value value;
                fvm_value_undefined(&value);
                value.type = FVM_VALUE_OBJECT;
                value.object = name;
                if (!(fvm_set_global(c, "name", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Bytes code;
                text(code, "name");
                code.push_back(0x1c);
                if (i == 1)
                {
                    if (!(fvm_object_create(c, &assigned) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value.object = assigned;
                    if (!(fvm_set_global(c, "assigned", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    text(code, "assigned");
                    code.push_back(0x1c);
                }
                code.push_back((unsigned char)ops[i]);
                if (i == 1)
                {
                    text(code, "result");
                    integer(code, 123);
                    code.push_back(0x1d);
                    code.push_back(0);
                }
                else
                {
                    finish(code);
                }
                converted = 0;
                FVM_GCRootFrame *roots = c->gc_frames;
                run(c, code);
                if (!(converted == 1 && roots == c->gc_frames))
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
