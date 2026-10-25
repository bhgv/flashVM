/* Loaded literal pops default to undefined while raw embedding remains strict. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_standard_types.cpp"
#include "flashvm/table.h"


static FVM_Value literal(FVM_Context *c, Bytes b)
{
    text(b, "result");
    b.push_back(0x4d);
    b.push_back(0x1d);
    b.push_back(0);
    if (!(fvm_avm1_execute(c, b.begin(), b.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value result;
    if (!(fvm_get_global(c, "result", &result) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(result.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return result;
}


static void own(FVM_Context *c, FVM_Object *o, const char *name)
{
    FVM_Value key, result;
    fvm_value_string(&key, name);
    if (!(fvm_object_call(c, o, "hasOwnProperty", 1, &key, &result) == FVM_OK && result.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *raw = 0;
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
    if (!(fvm_context_create(rt, &raw) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(raw) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
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
    for (unsigned op = 0x42; op <= 0x43; ++op)
    {
        Bytes b;
        b.push_back((unsigned char)op);
        FVM_Value result = literal(c, b), v;
        if (op == 0x42)
        {
            if (!(fvm_object_get(c, result.object, "length", &v) == FVM_OK && v.number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        b.clear();
        integer(b, 2);
        b.push_back((unsigned char)op);
        result = literal(c, b);
        if (op == 0x42)
        {
            if (!(fvm_object_get(c, result.object, "length", &v) == FVM_OK && v.number == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned i = 0; i < 2; ++i)
            {
                if (!(fvm_table_get_index(c, result.object, i, &v) == FVM_OK && v.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                own(c, result.object, i ? "1" : "0");
            }
        }
        else
        {
            own(c, result.object, "undefined");
        }
        b.push_back(0);
        if (!(fvm_avm1_execute(raw, b.begin(), b.size()) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char empty[] = {(unsigned char)op, 0};
        if (!(fvm_avm1_execute(raw, empty, sizeof(empty)) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Bytes b;
    text(b, "first");
    integer(b, 2);
    b.push_back(0x42);
    FVM_Value result = literal(c, b), v;
    if (!(fvm_table_get_index(c, result.object, 0, &v) == FVM_OK && v.type == FVM_VALUE_STRING &&
          !std::strcmp(v.string, "first")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_table_get_index(c, result.object, 1, &v) == FVM_OK && v.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    b.clear();
    text(b, "present");
    text(b, "kept");
    integer(b, 2);
    b.push_back(0x43);
    result = literal(c, b);
    if (!(fvm_object_get(c, result.object, "present", &v) == FVM_OK && v.type == FVM_VALUE_STRING &&
          !std::strcmp(v.string, "kept")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    own(c, result.object, "undefined");
    for (unsigned version = 6; version <= 8; ++version)
    {
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        b.clear();
        text(b, "value");
        integer(b, 1);
        b.push_back(0x43);
        result = literal(c, b);
        if (!(fvm_object_get(c, result.object, version < 7 ? "" : "undefined", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version < 7)
        {
            int own = 1;
            if (!(v.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_has_own_bytes(c, result.object, "", 0, &own) == FVM_OK && !own))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(v.type == FVM_VALUE_STRING && !std::strcmp(v.string, "value")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_context_destroy(raw);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
