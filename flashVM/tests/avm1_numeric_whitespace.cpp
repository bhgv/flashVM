/* Loaded Number/parseFloat/parseInt use Flash's four-unit leading whitespace set. */
#define FVM_PARSE_INT_LOADED_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "global_parse_int_loaded.cpp"
#include "flashvm/avm1.h"


static void whitespace_check(FVM_Context *c, unsigned version, unsigned short unit)
{
    unsigned short units[] = {unit, '4', '2'};
    FVM_Value input, out;
    if (!(fvm_value_string_utf16(c, units, 3, &input) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool accepted = unit == 9 || unit == 10 || unit == 13 || unit == 32;
    const char *names[] = {"Number", "parseFloat", "parseInt"};
    for (unsigned i = 0; i < 3; ++i)
    {
        if (!(fvm_call(c, names[i], 1, &input, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (accepted)
        {
            if (!(out.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (i == 0 && version == 4)
        {
            if (!(out.number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(out.number != out.number))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(fvm_set_global(c, "whitespaceInput", &input) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char number_action[] = {0x96, 17,  0,   0,   'w', 'h', 'i', 't', 'e',  's',  'p',  'a',
                                           'c',  'e', 'I', 'n', 'p', 'u', 't', 0,   0x1c, 0x4a, 0x3e, 0};
    FVM_Object *fn = 0;
    if (!(fvm_function_create_avm1(c, number_action, sizeof(number_action), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, fn) == FVM_OK && fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (accepted)
    {
        if (!(out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (version == 4)
    {
        if (!(out.number == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(out.number != out.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_unpin(c, fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_NUMERIC_WHITESPACE_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned short candidates[] = {9,      10,     13,     32,     0,      11,     12,     0x85,
                                         0xa0,   0x1680, 0x180e, 0x2000, 0x2001, 0x2002, 0x2003, 0x2004,
                                         0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200a, 0x2028, 0x2029,
                                         0x202f, 0x205f, 0x3000, 0xfeff, 0xd800, 0xdc00};
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        parsing_load(c, version);
        if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i)
        {
            whitespace_check(c, version, candidates[i]);
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
