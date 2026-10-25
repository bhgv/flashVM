/* Legacy ordinary undefined string conversion is distinct from Trace spelling. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_standard_types.cpp"
#include "../src/details/string.hpp"

static fvm_detail::string traced;


static FVM_Result sink(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *, void *)
{
    if (!(n == 1 && a[0].type == FVM_VALUE_STRING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    traced = fvm_detail::string(a[0].string, fvm_value_string_size(a));
    return FVM_OK;
}


static FVM_Value nullish_call(FVM_Context *c, const FVM_Value &owner)
{
    if (!(fvm_set_global(c, "owner", &owner) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes b;
    integer(b, 9);
    integer(b, 1);
    global(b, "owner");
    text(b, "toString");
    b.push_back(0x52);
    return run(c, b);
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
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
    for (unsigned version = 5; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 0, 16, 0, 0, 0, 0x08, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
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
        FVM_Value value;
        fvm_value_undefined(&value);
        if (!(fvm_set_global(c, "input", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes b;
        global(b, "input");
        b.push_back(0x4b);
        FVM_Value result = run(c, b);
        if (!(result.type == FVM_VALUE_STRING && !std::strcmp(result.string, version < 7 ? "" : "undefined")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        b.clear();
        text(b, "prefix");
        global(b, "input");
        b.push_back(0x47);
        result = run(c, b);
        if (!(result.type == FVM_VALUE_STRING &&
              !std::strcmp(result.string, version < 7 ? "prefix" : "prefixundefined")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        b.clear();
        global(b, "input");
        text(b, "suffix");
        b.push_back(0x47);
        result = run(c, b);
        if (!(result.type == FVM_VALUE_STRING &&
              !std::strcmp(result.string, version < 7 ? "suffix" : "undefinedsuffix")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_avm1_set_trace_rooted(c, sink, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        b.clear();
        global(b, "input");
        b.push_back(0x26);
        b.push_back(0);
        if (!(fvm_avm1_execute(c, b.begin(), b.size()) == FVM_OK && traced == fvm_detail::string("undefined")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(nullish_call(c, value).type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.type = FVM_VALUE_NULL;
        if (!(nullish_call(c, value).type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_set_global(c, "input", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        b.clear();
        global(b, "input");
        b.push_back(0x4b);
        if (!(!std::strcmp(run(c, b).string, "null")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Loaded calls consume available operands even when the requested count is larger. */
        b.clear();
        integer(b, 1);
        global(b, "owner");
        text(b, "method");
        b.push_back(0x52);
        b.push_back(0);
        if (!(fvm_avm1_execute(c, b.begin(), b.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
