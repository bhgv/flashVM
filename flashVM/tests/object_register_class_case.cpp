/* Versioned linkage comparisons preserve counted names and replacement spelling. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "../src/standard/base_state.hpp"
#include "../src/core/property_key.hpp"
#include <cstring>


static FVM_Value case_object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = o;
    return v;
}


static FVM_Object *case_ctor(FVM_Context *c, unsigned value)
{
    unsigned char code[] = {0x96, 5, 0, 7, 11, 0, 0, 0, 0x3e, 0};
    code[4] = (unsigned char)value;
    FVM_Object *fn = 0;
    if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fn;
}


static void case_register(FVM_Context *c, FVM_Object *klass, const char *key, size_t size, FVM_Object *fn)
{
    FVM_Value a[2], out;
    if (!(fvm_value_string_bytes(a, key, size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    a[1] = case_object(fn);
    if (!fn)
    {
        a[1].type = FVM_VALUE_NULL;
    }
    if (!(fvm_object_call(c, klass, "registerClass", 2, a, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN &&
          out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void case_loaded(FVM_Context *c, unsigned version)
{
    if (version)
    {
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}
#ifndef FVM_CLASS_CASE_FIXTURE_ONLY


int main()
{
    const char *upper[] = {"Hero",
                           "\xc3\x84"
                           "Case",
                           "\xd0\x94\xd0\x9e\xd0\x9c",
                           "\xce\xa3",
                           "\xc4\xb0",
                           "A\0B",
                           "\xed\xa0\x80"
                           "A",
                           "\xf0\x90\x80\x80"
                           "A"};
    const char *lower[] = {"hERO",
                           "\xc3\xa4"
                           "cASE",
                           "\xd0\xb4\xd0\xbe\xd0\xbc",
                           "\xcf\x83",
                           "i",
                           "a\0b",
                           "\xed\xa0\x80"
                           "a",
                           "\xed\xa0\x80\xed\xb0\x80"
                           "a"};
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
        bool folded = version && version < 7;
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        case_loaded(c, version);
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value klass;
        if (!(fvm_get_global(c, "Object", &klass) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *one = case_ctor(c, 11), *two = case_ctor(c, 22), *found = 0, *registry = 0;
        if (!(fvm_base_class_registry(c, &registry) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (c->swf_loaded && c->swf_info.version < 7)
        {
            registry = registry->indexed.find(0)->second.object;
        }
        for (unsigned i = 0; i < 8; ++i)
        {
            size_t us = i == 5 ? 3 : std::strlen(upper[i]), ls = i == 5 ? 3 : std::strlen(lower[i]);
            case_register(c, klass.object, upper[i], us, one);
            if (!(fvm_registered_class_bytes(c, lower[i], ls, &found) == (folded ? FVM_OK : FVM_NOT_FOUND)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (folded)
            {
                if (!(found == one))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            case_register(c, klass.object, lower[i], ls, two);
            if (!(registry->properties.size() == (folded ? 1U : 2U)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_registered_class_bytes(c, upper[i], us, &found) == FVM_OK && found == (folded ? two : one)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (folded)
            {
                fvm_detail::string stored;
                if (!(fvm_property_key(lower[i], ls, stored) == FVM_OK &&
                      registry->properties.find(stored) != registry->properties.end()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            case_register(c, klass.object, upper[i], us, 0);
            if (!(fvm_registered_class_bytes(c, lower[i], ls, &found) == (folded ? FVM_NOT_FOUND : FVM_OK)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            case_register(c, klass.object, lower[i], ls, 0);
            if (!(registry->properties.empty()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        /* Supplementary Deseret letters stay distinct because Flash folds UTF-16 units. */
        case_register(c, klass.object, "\xf0\x90\x90\x80", 4, one);
        if (!(fvm_registered_class_bytes(c, "\xf0\x90\x90\xa8", 4, &found) == FVM_NOT_FOUND))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        case_register(c, klass.object, "Mixed", 5, one);
        FVM_Channel *ch = 0;
        FVM_Context *other = 0;
        FVM_Value copy;
        if (!(fvm_channel_create(&ch) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        case_loaded(other, version);
        if (!(fvm_channel_send_value(c, ch, &klass, 0) == FVM_OK &&
              fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_set_global(other, "Object", &copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_registered_class(other, "mIXED", &found) == (folded ? FVM_OK : FVM_NOT_FOUND)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (folded)
        {
            FVM_Value out;
            if (!(fvm_function_invoke(other, found, 0, 0, 0, &out) == FVM_OK && out.number == 11))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            case_register(other, copy.object, "MIXED", 5, 0);
            if (!(fvm_registered_class(other, "Mixed", &found) == FVM_NOT_FOUND))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_registered_class(c, "Mixed", &found) == FVM_OK && found == one))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->gc_frames && !c->gc_native_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(ch);
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
