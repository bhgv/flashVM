/* Inherited indexed setters do not grow a loaded Array; own setters observe prior growth. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "flashvm/table.h"
#include "../src/core/runtime.hpp"

static unsigned index_calls, index_mode;
static double expected_length;


static FVM_Result index_get(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result index_watch(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *, void *)
{
    if (!(frame->argc == 4 && frame->argv[1].type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value error;
    fvm_value_number(&error, 77);
    return fvm_exception_raise(c, &error);
}


static FVM_Result index_set(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++index_calls;
    if (!(frame->argc == 1 && !c->exception_pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (index_mode >= 2)
    {
        if (!(frame->argv[0].type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(frame->argv[0].number == 81))
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
    FVM_Value length;
    if (!(fvm_object_get(c, frame->receiver, "length", &length) == FVM_OK && length.number == expected_length))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (index_mode & 1)
    {
        fvm_value_number(out, 88);
        return fvm_exception_raise(c, out);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Value index_object(FVM_Object *object)
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
        for (unsigned own = 0; own < 2; ++own)
        {
            for (index_mode = 0; index_mode < 4; ++index_mode)
            {
                if ((version && version < 4) || (!version && index_mode >= 2))
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
                if (!(fvm_standard_install(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *array = 0, *parent = 0, *get = 0, *set = 0, *watch = 0;
                FVM_Value args[3], value, out;
                if (!(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK &&
                      fvm_object_set_prototype(c, parent, array->prototype) == FVM_OK &&
                      fvm_object_set_prototype(c, array, parent) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const unsigned char end[] = {0};
                if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK &&
                      fvm_function_set_body(c, get, index_get, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &set) == FVM_OK &&
                      fvm_function_set_body(c, set, index_set, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_string(args, "5");
                args[1] = index_object(get);
                args[2] = index_object(set);
                if (!version && own)
                {
                    fvm_context_destroy(c);
                    continue;
                }
                if (!(fvm_object_call(c, own ? array : parent, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (index_mode >= 2)
                {
                    if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &watch) == FVM_OK &&
                          fvm_function_set_body(c, watch, index_watch, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    args[1] = index_object(watch);
                    if (!(fvm_object_call(c, array, "watch", 2, args, &out) == FVM_OK && out.boolean))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                index_calls = 0;
                expected_length = version && own ? 6 : 0;
                fvm_value_number(&value, 81);
                FVM_Result status = fvm_table_set_index(c, array, 5, &value);
                bool error = (index_mode & 1) || (index_mode >= 2 && own);
                if (!(status == (error ? FVM_EXCEPTION : FVM_OK) && index_calls == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (error)
                {
                    if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == (index_mode >= 2 && own ? 77 : 88)))
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
                if (!(fvm_object_get(c, array, "length", &out) == FVM_OK && out.number == expected_length))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                int present = 0;
                if (!(fvm_object_has_own(c, array, "5", &present) == FVM_OK && !!present == !!own))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner && c->watch_running.empty()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
