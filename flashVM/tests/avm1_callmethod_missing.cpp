/* Missing loaded methods do not use cached scripts; primitive receivers use current globals. */
#define main function2_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_function2.cpp"

#undef main
#include "../src/core/runtime.hpp"
#include "flashvm/execution.h"


static void definition(Bytes &b, const char *key)
{
    Bytes body;
    integer(body, 99);
    body.push_back(0x3e);
    body.push_back(0);
    Bytes header;
    name(header, key);
    u16(header, 0);
    u16(header, (unsigned)body.size());
    b.push_back(0x9b);
    u16(b, (unsigned)header.size());
    for (size_t i = 0; i < header.size(); ++i)
    {
        b.push_back(header[i]);
    }
    for (size_t i = 0; i < body.size(); ++i)
    {
        b.push_back(body[i]);
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
        Bytes code;
        definition(code, "cached");
        code.push_back(0);
        if (!(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK &&
              c->script_functions.find("cached") != c->script_functions.end()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *receiver = 0;
        if (!(fvm_object_create(c, &receiver) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value owner, key, out;
        fvm_value_undefined(&owner);
        owner.type = FVM_VALUE_OBJECT;
        owner.object = receiver;
        fvm_value_string(&key, "cached");
        if (!(fvm_exec_call_method(c, 0, &owner, &key, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        owner.type = FVM_VALUE_UNDEFINED;
        if (!(fvm_set_global(c, "Number", &owner) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes body;
        integer(body, 0);
        integer(body, 42);
        text(body, "missing");
        body.push_back(0x52);
        body.push_back(0x3e);
        body.push_back(0);
        FVM_Object *fn = 0;
        if (!(fvm_function_create_avm1(c, body.begin(), body.size(), 0, 0, 0, &fn) == FVM_OK &&
              fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
