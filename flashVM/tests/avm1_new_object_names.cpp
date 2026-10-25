/* Loaded NewObject resolves literal names, unlike CallFunction/GetVariable path syntax. */
#define main super_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_super.cpp"

#undef main
#include "flashvm/execution.h"
#include "flashvm/property.h"
#include "../src/core/runtime.hpp"


static FVM_Value construct_name(FVM_Context *c, const char *key)
{
    Bytes code;
    integer(code, 0);
    text(code, key);
    code.push_back(0x40);
    result(code);
    FVM_Object *fn = 0;
    FVM_Value out;
    if (!(fvm_function_create_avm1(c, code.begin(), code.size(), 0, 0, 0, &fn) == FVM_OK &&
          fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK))
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (version)
        {
            swf[3] = (unsigned char)version;
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
        Bytes body;
        variable(body, "this");
        text(body, "marker");
        integer(body, 99);
        body.push_back(0x4f);
        body.push_back(0);
        FVM_Value ctor = define(c, "Ctor", 0, 0, body);
        Bytes literal;
        variable(literal, "this");
        text(literal, "marker");
        integer(literal, 77);
        literal.push_back(0x4f);
        literal.push_back(0);
        FVM_Value named = define(c, "LiteralCtor", 0, 0, literal);
        if (!(fvm_set_global(c, "space.Ctor", &named) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *space = 0;
        if (!(fvm_object_create(c, &space) == FVM_OK && fvm_object_set(c, space, "Ctor", &ctor) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value, out;
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = space;
        if (!(fvm_set_global(c, "space", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        out = construct_name(c, "space.Ctor");
        if (!(out.type == FVM_VALUE_OBJECT && fvm_object_get(c, out.object, "marker", &value) == FVM_OK &&
              value.number == (version ? 77 : 99)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        out = construct_name(c, "space:Ctor");
        if (!(out.type == (version ? FVM_VALUE_UNDEFINED : FVM_VALUE_OBJECT)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            int removed = 0;
            if (!(fvm_object_delete(c, c->global, "Ctor", &removed) == FVM_OK && removed &&
                  c->script_functions.find("Ctor") != c->script_functions.end()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(construct_name(c, "Ctor").type == FVM_VALUE_UNDEFINED &&
                  construct_name(c, "missing").type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 7);
            if (!(fvm_set_global(c, "scalar", &value) == FVM_OK &&
                  construct_name(c, "scalar").type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(c->lexical_scopes.empty() && !c->gc_frames && !c->calling_constructor))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        (void)named;
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
