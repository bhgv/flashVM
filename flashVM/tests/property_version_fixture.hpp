/* Independent AVM1 UInt16 attribute and visibility oracles. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_TEST_PROPERTY_VERSION_FIXTURE_HPP
#define FVM_TEST_PROPERTY_VERSION_FIXTURE_HPP
#include "primitive_boxing_fixture.hpp"

static unsigned version_masks[] = {0x7ff8, 0x7480, 0x7500, 0x7000, 0x6000};


static void version_load(FVM_Context *c, unsigned version)
{
    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    swf[3] = (unsigned char)version;
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void
version_flags(FVM_Context *c, FVM_Object *object, const char *name, unsigned set, unsigned clear, unsigned route = 0)
{
    FVM_Value args[4], out;
    args[0] = class_value(object);
    fvm_value_string(args + 1, name);
    fvm_value_number(args + 2, set);
    fvm_value_number(args + 3, clear);
    if (!route)
    {
        if (!(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(fvm_set_global(c, "versionTarget", args) == FVM_OK &&
              fvm_set_global(c, "versionName", args + 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        class_mode = route - 1;
        Program code;
        code.number(clear);
        code.number(set);
        code.variable("versionName");
        code.variable("versionTarget");
        code.number(4);
        code.text("ASSetPropFlags");
        code.op(0x3d);
        code.finish();
        out = boxing_run(c, code, route - 1, FVM_OK);
    }
    if (!(out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Object *version_function(FVM_Context *c, unsigned value)
{
    const unsigned char body[] = {0x96, 5, 0, 7, (unsigned char)value, 0, 0, 0, 0x3e, 0};
    FVM_Object *function = 0;
    if (!(fvm_function_create_avm1(c, body, sizeof(body), 0, 0, 0, &function) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return function;
}


static void version_enumerable(FVM_Context *c, FVM_Object *object, const char *name, bool expected)
{
    FVM_Object *keys = 0;
    FVM_Value length, key;
    if (!(fvm_object_enumerate(c, object, 0, &keys) == FVM_OK && fvm_object_get(c, keys, "length", &length) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool found = false;
    for (unsigned long i = 0; i < (unsigned long)length.number; ++i)
    {
        if (!(fvm_table_get_index(c, keys, i, &key) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (key.type == FVM_VALUE_STRING && !std::strcmp(key.string, name))
        {
            found = true;
        }
    }
    if (!(found == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#endif
