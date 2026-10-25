#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"


static void equal(const FVM_Value &value, const unsigned short *expected, size_t count)
{
    unsigned short units[16];
    size_t n = 0;
    if (!(fvm_value_get_utf16(&value, units, 16, &n) == FVM_OK && n == count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(units, expected, count * sizeof(*expected))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void casing(FVM_Context *c,
                   const unsigned short *input,
                   size_t n,
                   const unsigned short *upper,
                   size_t nu,
                   const unsigned short *lower,
                   size_t nl)
{
    FVM_Value value, out;
    FVM_Object *str = 0, *retained = 0;
    if (!(fvm_value_string_utf16(c, input, n, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "String", 1, &value, &str) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, str, "toUpperCase", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    equal(out, upper, nu);
    if (!(fvm_object_call(c, str, "toLowerCase", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    equal(out, lower, nl);
    if (!(fvm_construct(c, "String", 1, &out, &retained) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, retained) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, retained, "valueOf", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    equal(out, lower, nl);
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
    const unsigned short sharp[] = {0xdf}, ss[] = {'S', 'S'}, dotted[] = {0x130}, i[] = {'i'}, idot[] = {'i', 0x307};
    const unsigned short greek[] = {0x39f, 0x3a3}, lower_greek[] = {0x3bf, 0x3c3}, lower_host[] = {0x3bf, 0x3c2};
    const unsigned short sigma[] = {0x3c2}, sigma_flash[] = {0x3a2}, sigma_host[] = {0x3a3};
    const unsigned short ligature[] = {0xfb03}, ffi[] = {'F', 'F', 'I'};
    const unsigned short astral[] = {0xd801, 0xdc00}, astral_lower[] = {0xd801, 0xdc28};
    const unsigned short counted[] = {'A', 0, 0xd800, 'B', 0xdfff}, counted_lower[] = {'a', 0, 0xd800, 'b', 0xdfff};
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
        if (version)
        {
            unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
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
        casing(c, sharp, 1, version ? sharp : ss, version ? 1 : 2, sharp, 1);
        casing(c, dotted, 1, dotted, 1, version ? i : idot, version ? 1 : 2);
        casing(c, greek, 2, greek, 2, version ? lower_greek : lower_host, 2);
        casing(c, sigma, 1, version ? sigma_flash : sigma_host, 1, sigma, 1);
        casing(c, ligature, 1, version ? ligature : ffi, version ? 1 : 3, ligature, 1);
        casing(c, astral, 2, astral, 2, version ? astral : astral_lower, 2);
        casing(c, counted, 5, counted, 5, counted_lower, 5);
        FVM_Value values[2], flags, out;
        FVM_Object *array = 0;
        if (!(fvm_value_string_utf16(c, dotted, 1, values) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string_utf16(c, i, 1, values + 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Array", 2, values, &array) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&flags, 5);
        if (!(fvm_object_call(c, array, "sort", 1, &flags, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(version ? out.type == FVM_VALUE_NUMBER && out.number == 0 : out.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->gc_frames == 0 && c->avm1_execution_depth == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
