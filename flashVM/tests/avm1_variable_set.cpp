/* Assignment bypasses old-value getters and uses scoped paths across SWF4–8. */
#define main function2_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_function2.cpp"

#undef main
#include "../src/core/runtime.hpp"

static unsigned writes;


static FVM_Result forbidden(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    if (!(0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return FVM_ERROR;
}


static FVM_Result setter(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    if (!(n == 1 && a[0].number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++writes;
    fvm_value_undefined(out);
    return FVM_OK;
}


static void assign(FVM_Context *c, const char *key)
{
    Bytes code;
    text(code, key);
    integer(code, 42);
    code.push_back(0x1d);
    code.push_back(0);
    if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
        FVM_Object *scope = 0, *leaf = 0;
        if (!(fvm_object_create(c, &scope) == FVM_OK && fvm_object_create(c, &leaf) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = scope;
        if (!(fvm_set_global(c, "scope", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = leaf;
        if (!(fvm_set_global(c, "leaf", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_accessor_rooted(c, scope, "value", forbidden, setter, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes body;
        text(body, "value");
        integer(body, 42);
        body.push_back(0x1d);
        Bytes code;
        variable(code, "scope");
        code.push_back(0x94);
        u16(code, 2);
        u16(code, (unsigned)body.size());
        for (size_t i = 0; i < body.size(); ++i)
        {
            code.push_back(body[i]);
        }
        code.push_back(0);
        if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        assign(c, "leaf.value");
        if (!(fvm_object_get(c, leaf, "value", &value) == FVM_OK && value.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        assign(c, "literal/slash");
        if (!(fvm_get_global(c, "literal/slash", &value) == FVM_OK && value.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        assign(c, "missing.value");
        if (!(fvm_get_global(c, "missing.value", &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        assign(c, "");
        if (!(fvm_get_global(c, "", &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(writes == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
