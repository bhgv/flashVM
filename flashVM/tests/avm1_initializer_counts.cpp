/* Loaded initializer counts coerce, truncate and reject ranges without consuming values. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/avm1.h"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include "../src/core/runtime.hpp"
#include <cstring>

typedef fvm_detail::vector<unsigned char> Bytes;


static void text(Bytes &b, const char *s)
{
    size_t n = std::strlen(s) + 2;
    b.push_back(0x96);
    b.push_back((unsigned char)n);
    b.push_back((unsigned char)(n >> 8));
    b.push_back(0);
    do
    {
        b.push_back((unsigned char)*s);
    } while (*s++);
}


static void global(Bytes &b, const char *name)
{
    text(b, name);
    b.push_back(0x1c);
}


static FVM_Result run(FVM_Context *c, unsigned op, const FVM_Value &count, bool with_value)
{
    if (!(fvm_set_global(c, "count", &count) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes code;
    text(code, "result");
    if (with_value)
    {
        if (op == 0x43)
        {
            text(code, "key");
        }
        text(code, "value");
    }
    global(code, "count");
    code.push_back((unsigned char)op);
    code.push_back(0x1d);
    code.push_back(0);
    return fvm_avm1_execute(c, code.begin(), code.size());
}

static unsigned traces;


static FVM_Result trace(FVM_Context *, FVM_Object *, unsigned int n, const FVM_Value *args, FVM_Value *out, void *)
{
    if (!(n == 1 && args[0].type == FVM_VALUE_STRING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *expected[] = {"undefined", "c", "b", "a"};
    if (!(traces < 4 && !std::strcmp(args[0].string, expected[traces++])))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_avm1_set_trace_rooted(c, trace, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned op = 0x42; op <= 0x43; ++op)
    {
        FVM_Value count, out;
        fvm_value_number(&count, 4294967296.0);
        if (!(fvm_set_global(c, "count", &count) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes code;
        text(code, "a");
        text(code, "b");
        text(code, "c");
        global(code, "count");
        code.push_back((unsigned char)op);
        for (unsigned i = 0; i < 4; ++i)
        {
            code.push_back(0x26);
        }
        code.push_back(0);
        traces = 0;
        if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK && traces == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&count, -0.5);
        if (!(run(c, op, count, false) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "result", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&count, "1.9");
        if (!(run(c, op, count, true) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "result", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        if (op == 0x42)
        {
            if (!(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(fvm_object_get(c, out.object, "key", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "value")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&count);
        if (!(run(c, op, count, false) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "result", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&count, 65537);
        if (!(run(c, op, count, false) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
