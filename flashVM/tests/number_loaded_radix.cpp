/* Loaded nondecimal Number formatting uses Int32 truncation and the legacy MIN sentinel. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include "../src/core/runtime.hpp"
#include "number_loaded_radix_expected.h"
#include <limits>
#include <cfloat>
#include <cmath>


static void radix_loaded(FVM_Context *c, unsigned version)
{
    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    swf[3] = (unsigned char)version;
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Object *radix_box(FVM_Context *c, double n)
{
    FVM_Value value;
    fvm_value_number(&value, n);
    FVM_Object *o = 0;
    if (!(fvm_construct(c, "Number", 1, &value, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return o;
}


static void radix_expect(FVM_Context *c, double n, unsigned radix, const char *expected)
{
    FVM_Object *o = radix_box(c, n);
    FVM_Value arg, out, stored;
    fvm_value_number(&arg, radix);
    if (!(fvm_object_call(c, o, "toString", 1, &arg, &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
          fvm_value_string_size(&out) == std::strlen(expected) &&
          !std::memcmp(out.string, expected, std::strlen(expected))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, o, "valueOf", 0, 0, &stored) == FVM_OK && stored.type == FVM_VALUE_NUMBER &&
          (n != n ? stored.number != stored.number : stored.number == n)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_frames && !c->gc_native_depth && !c->primitive_depth && !c->string_depth && !c->avm1_execution_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_LOADED_RADIX_FIXTURE_ONLY


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
        radix_loaded(c, version);
        const double exceptional[] = {-2147483648.0,
                                      2147483647.5,
                                      2147483648.0,
                                      -2147483648.5,
                                      -2147483649.0,
                                      1e100,
                                      -1e100,
                                      DBL_MAX,
                                      -DBL_MAX,
                                      std::numeric_limits<double>::quiet_NaN(),
                                      std::numeric_limits<double>::infinity(),
                                      -std::numeric_limits<double>::infinity()};
        for (unsigned i = 0; i < sizeof(loaded_radix_expected) / sizeof(*loaded_radix_expected); ++i)
        {
            const LoadedRadixExpected &e = loaded_radix_expected[i];
            char negative_max[34];
            negative_max[0] = '-';
            std::strcpy(negative_max + 1, e.maximum);
            radix_expect(c, -2147483647.0, e.radix, negative_max);
            radix_expect(c, 2147483647.0, e.radix, e.maximum);
            radix_expect(c, 42.9, e.radix, e.positive);
            radix_expect(c, -42.9, e.radix, e.negative);
            radix_expect(c, 0.25, e.radix, "0");
            radix_expect(c, -0.25, e.radix, "0");
            radix_expect(c, -0.0, e.radix, "0");
            radix_expect(c, std::numeric_limits<double>::min(), e.radix, "0");
            for (unsigned j = 0; j < sizeof(exceptional) / sizeof(*exceptional); ++j)
            {
                radix_expect(c, exceptional[j], e.radix, e.sentinel);
            }
        }
        radix_expect(c, 2147483646.5, 16, "7ffffffe");
        radix_expect(c, -2147483647.5, 16, "-7fffffff");
        radix_expect(c, std::ldexp(1.0, -1074), 16, "0");
        radix_expect(c, 0.25, 10, "0.25");
        radix_expect(c, 2147483647.5, 10, "2147483647.5");
        radix_expect(c, std::numeric_limits<double>::infinity(), 10, "Infinity");
        radix_expect(c, std::numeric_limits<double>::quiet_NaN(), 10, "NaN");
        FVM_Object *number = radix_box(c, -2147483648.0);
        if (!(fvm_gc_pin(c, number) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value arg, out;
        fvm_value_number(&arg, 36);
        if (!(fvm_object_call(c, number, "toString", 1, &arg, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_GCRootScope *roots = 0;
        if (!(fvm_gc_roots_begin(c, &roots) == FVM_OK && fvm_gc_roots_add(roots, &out) == FVM_OK &&
              fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const char *expected = loaded_radix_expected[33].sentinel;
        if (!(fvm_value_string_size(&out) == std::strlen(expected) &&
              !std::memcmp(out.string, expected, std::strlen(expected))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_roots_end(roots) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *transfer_number = radix_box(c, -10.625);
        if (!(fvm_gc_pin(c, transfer_number) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value boxed = object(transfer_number);
        for (unsigned loaded = 0; loaded < 2; ++loaded)
        {
            FVM_Context *other = 0;
            FVM_Channel *channel = 0;
            FVM_Value copy;
            if (!(fvm_context_create(rt, &other) == FVM_OK && fvm_channel_create(&channel) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (loaded)
            {
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                if (!(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_channel_send_value(c, channel, &boxed, 0) == FVM_OK &&
                  fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK &&
                  fvm_gc_pin(other, copy.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&arg, 16);
            if (!(fvm_object_call(other, copy.object, "toString", 1, &arg, &out) == FVM_OK &&
                  !std::strcmp(out.string, loaded ? "-a" : "-a.a")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_channel_destroy(channel);
            fvm_context_destroy(other);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
