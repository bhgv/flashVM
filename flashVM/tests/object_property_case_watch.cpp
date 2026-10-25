/* Watch lookup preserves requested spelling and raw folded old values under collecting failures. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cstring>

static unsigned watch_calls, watch_fault;


static FVM_Result case_watch(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++watch_calls;
    if (!(frame->argc == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string_size(frame->argv) == 10 && !std::memcmp(frame->argv[0].string, "mixed\0name", 10)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame->argv[1].type == FVM_VALUE_NUMBER && frame->argv[1].number == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame->argv[2].number == 81 && frame->argv[3].number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (watch_fault == 3)
    {
        FVM_Value key, ignored;
        fvm_value_string_bytes(&key, "MIXED\0NAME", 10);
        if (!(fvm_object_call(c, frame->receiver, "unwatch", 1, &key, &ignored) == FVM_OK && ignored.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (watch_fault == 1)
    {
        return FVM_ERROR;
    }
    if (watch_fault == 2)
    {
        FVM_Value error;
        fvm_value_string_bytes(&error, "watch\0failed", 12);
        return fvm_exception_raise(c, &error);
    }
    fvm_value_number(out, 82);
    return FVM_OK;
}


static FVM_Value watch_object(FVM_Object *object)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    return value;
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
        if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *fn = 0;
        const unsigned char end[] = {0};
        if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &fn) == FVM_OK &&
              fvm_function_set_body(c, fn, case_watch, 0, 0) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const bool folded = version && version <= 6;
        for (watch_fault = 0; watch_fault < 4; ++watch_fault)
        {
            FVM_Object *object = 0;
            FVM_Value value, args[3], out;
            if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 9);
            if (!(fvm_object_set_bytes(c, object, "MiXeD\0Name", 10, &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string_bytes(args, "MiXeD\0Name", 10);
            args[1] = watch_object(fn);
            fvm_value_number(args + 2, 42);
            if (!(fvm_object_call(c, object, "watch", 3, args, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (folded)
            {
                fvm_value_string_bytes(args, "MIXED\0NAME", 10);
                if (!(fvm_object_call(c, object, "watch", 3, args, &out) == FVM_OK && out.boolean &&
                      object->watchers.size() == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            watch_calls = 0;
            fvm_value_number(&value, 81);
            FVM_Result status = fvm_object_set_bytes(c, object, "mixed\0name", 10, &value);
            if (!(watch_calls == (folded ? 1U : 0U)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (folded && (watch_fault == 1 || watch_fault == 2))
            {
                if (!(status == (watch_fault == 1 ? FVM_ERROR : FVM_EXCEPTION)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (watch_fault == 2)
                {
                    if (!(fvm_exception_get(c, &out) == FVM_OK && fvm_value_string_size(&out) == 12 &&
                          !std::memcmp(out.string, "watch\0failed", 12)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_exception_clear(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                if (!(status == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_object_get_bytes(c, object, "MiXeD\0Name", 10, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (folded && watch_fault == 2)
            {
                if (!(out.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(out.type == FVM_VALUE_NUMBER && out.number == (folded && watch_fault != 1 ? 82 : 9)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(!c->gc_frames && !c->gc_native_depth && c->watch_running.empty()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (watch_fault != 3 || !folded)
            {
                fvm_value_string_bytes(args, "MIXED\0NAME", 10);
                if (!(fvm_object_call(c, object, "unwatch", 1, args, &out) == FVM_OK && !!out.boolean == folded))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(object->watchers.empty()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
