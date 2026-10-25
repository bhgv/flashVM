/* Literal Array keys and native length classification have distinct whitespace rules. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"


struct IndexWhitespaceCase
{
    const char *key;
    size_t size;
    double loaded_length;
};


static void whitespace_case(
    FVM_Runtime *rt, unsigned version, const char *key, size_t size, double expected, unsigned route, bool transport)
{
    FVM_Context *c = 0;
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (version)
    {
        version_load(c, version);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *array = 0;
    if (!(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value target = class_value(array), name, value, out;
    if (!(fvm_set_global(c, "whitespaceTarget", &target) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string_bytes(&name, key, size) == FVM_OK && fvm_set_global(c, "whitespaceKey", &name) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 81);
    if (!route)
    {
        if (!(fvm_object_set_bytes(c, array, key, size, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        Program body;
        body.variable("whitespaceTarget");
        body.variable("whitespaceKey");
        body.number(81);
        body.op(0x4f);
        body.number(1);
        body.finish();
        out = boxing_run(c, body, route - 1, FVM_OK);
    }
    if (!(fvm_object_get(c, array, "length", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get_bytes(c, array, key, size, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
          out.number == 81))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int own = 0;
    if (!(fvm_object_has_own_bytes(c, array, key, size, &own) == FVM_OK && own))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* A classified noncanonical key affects length, but never aliases canonical element 3. */
    if (!(fvm_table_get_index(c, array, 3, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (transport)
    {
        unsigned char *wire = 0;
        size_t bytes = 0;
        FVM_Context *other = 0;
        if (!(fvm_value_serialize(c, &target, &wire, &bytes) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            version_load(other, version);
        }
        if (!(fvm_standard_install(other) == FVM_OK && fvm_value_deserialize(other, wire, bytes, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(wire);
        FVM_Object *copy = out.object;
        if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK &&
              fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(other, copy, "length", &out) == FVM_OK && out.number == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get_bytes(other, copy, key, size, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
              out.number == 81))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_table_get_index(other, copy, 3, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(other);
    }
    if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
}
#ifndef FVM_ARRAY_INDEX_WHITESPACE_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const IndexWhitespaceCase cases[] = {{"\v3", 2, 0},
                                         {" \t\n\r\f+003", 9, 4},
                                         {" \v3", 3, 0},
                                         {"\v 3", 3, 0},
                                         {"\xc2\x85"
                                          "3",
                                          3,
                                          0},
                                         {"\xc2\xa0"
                                          "3",
                                          3,
                                          0},
                                         {"\xe2\x80\x83"
                                          "3",
                                          4,
                                          0},
                                         {"\xef\xbb\xbf"
                                          "3",
                                          4,
                                          0},
                                         {"\t3\0", 3, 0},
                                         {"\t3\0x", 4, 0},
                                         {"\t+4294967299", 12, 4},
                                         {"\n-4294967293", 12, 4},
                                         {"\r+2147483647", 12, -2147483648.0},
                                         {"\f-2147483649", 12, -2147483648.0},
                                         {"\t18446744073709551619", 21, 4},
                                         {"\t3 ", 3, 0},
                                         {"\t3\v", 3, 0},
                                         {"\t+", 2, 0},
                                         {"\t-", 2, 0},
                                         {"\t0x3", 4, 0},
                                         {"\t3.0", 4, 0}};
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        for (unsigned route = 0; route < 4; ++route)
        {
            if (route >= 2 && !fvm_jit_available())
            {
                continue;
            }
            for (unsigned control = 0; control <= 33; ++control)
            {
                char key[] = {(char)(control == 33 ? 127 : control), '3'};
                const bool whitespace =
                    control == 9 || control == 10 || control == 12 || control == 13 || control == 32;
                whitespace_case(rt, version, key, sizeof(key), version && whitespace ? 4 : 0, route, false);
            }
            for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
            {
                whitespace_case(
                    rt, version, cases[i].key, cases[i].size, version ? cases[i].loaded_length : 0, route, true);
            }
        }
    }
    fvm_runtime_destroy(rt);
}

#endif
