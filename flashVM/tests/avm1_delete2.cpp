/* Delete2 covers globals, activation locals and With shadowing for SWF4–8. */
#define main function2_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_function2.cpp"

#undef main
#include "flashvm/execution.h"
#include "flashvm/property.h"


static FVM_Value run_delete(FVM_Context *c, const char *key, bool local)
{
    Bytes bytes;
    if (local)
    {
        text(bytes, key);
        integer(bytes, 17);
        bytes.push_back(0x3c);
    }
    text(bytes, key);
    bytes.push_back(0x3b);
    bytes.push_back(0x3e);
    bytes.push_back(0);
    FVM_Object *fn = 0;
    if (!(fvm_function_create_avm1(c, bytes.begin(), bytes.size(), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value out;
    if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out;
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
        unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value;
        fvm_value_number(&value, 42);
        if (!(fvm_set_global(c, "x", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(run_delete(c, "x", false).boolean && fvm_get_global(c, "x", &value) == FVM_OK &&
              value.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!run_delete(c, "x", false).boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 42);
        if (!(fvm_set_global(c, "x", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(run_delete(c, "x", true).boolean && fvm_get_global(c, "x", &value) == FVM_OK && value.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *o = 0, *p = 0;
        if (!(fvm_object_create(c, &o) == FVM_OK && fvm_object_create(c, &p) == FVM_OK &&
              fvm_object_set_prototype(c, o, p) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, p, "x", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.type = FVM_VALUE_OBJECT;
        value.object = o;
        if (!(fvm_set_global(c, "scope", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes inner;
        text(inner, "x");
        inner.push_back(0x3b);
        inner.push_back(0x3e);
        Bytes outer;
        variable(outer, "scope");
        outer.push_back(0x94);
        u16(outer, 2);
        u16(outer, (unsigned)inner.size());
        for (size_t i = 0; i < inner.size(); ++i)
        {
            outer.push_back(inner[i]);
        }
        outer.push_back(0);
        FVM_Object *fn = 0;
        if (!(fvm_function_create_avm1(c, outer.begin(), outer.size(), 0, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_invoke(c, fn, 0, 0, 0, &value) == FVM_OK && value.type == FVM_VALUE_BOOLEAN &&
              !value.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "x", &value) == FVM_OK && value.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
