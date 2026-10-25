/* Loaded thrown watchers write undefined before propagating; inherited setters consume the error. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include "../src/standard/base_state.hpp"

static FVM_Object *watch_thrown;
static unsigned setter_calls, setter_throws;


static FVM_Value thrown_object(FVM_Object *object)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    return value;
}


static FVM_Result throwing_watch(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *, void *)
{
    if (!(frame->argc == 4 && frame->argv[1].number == 9 && frame->argv[2].number == 81))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &watch_thrown) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value = thrown_object(watch_thrown);
    if (!(fvm_object_set(c, watch_thrown, "self", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 77);
    if (!(fvm_object_set(c, watch_thrown, "marker", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = thrown_object(watch_thrown);
    return fvm_exception_raise(c, &value);
}


static FVM_Result throw_get(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    fvm_value_number(out, 17);
    return FVM_OK;
}


static FVM_Result throw_set(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++setter_calls;
    if (!(frame->argc == 1 && frame->argv[0].type == FVM_VALUE_UNDEFINED && !c->exception_pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool alive = false;
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        if (c->objects[i] == watch_thrown)
        {
            alive = true;
        }
    }
    if (!(alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    if (!(fvm_object_get(c, watch_thrown, "marker", &value) == FVM_OK && value.number == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (setter_throws)
    {
        fvm_value_number(&value, 88);
        return fvm_exception_raise(c, &value);
    }
    fvm_value_undefined(out);
    return FVM_OK;
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
        for (unsigned kind = 0; kind < 6; ++kind)
        {
            for (setter_throws = 0; setter_throws < 2; ++setter_throws)
            {
                if ((version && version < 4) || (!version && (kind == 3 || kind == 4)))
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
                FVM_Object *object = 0, *owner = 0, *watch = 0, *get = 0, *set = 0;
                FVM_Value value, args[3], out;
                const char *key = kind == 1 || kind == 2 ? "0" : "slot";
                if (!(fvm_construct(c, kind == 1 ? "Array" : "Object", 0, 0, &object) == FVM_OK &&
                      fvm_gc_pin(c, object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                owner = object;
                if (kind == 4)
                {
                    if (!(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK &&
                          fvm_object_set_prototype(c, object, owner) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                fvm_value_number(&value, 9);
                if (!(fvm_object_set(c, owner, key, &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const unsigned char end[] = {0};
                if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &watch) == FVM_OK &&
                      fvm_function_set_body(c, watch, throwing_watch, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (kind == 3 || kind == 4)
                {
                    if (!(fvm_gc_pin(c, watch) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK &&
                          fvm_function_set_body(c, get, throw_get, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &set) == FVM_OK &&
                          fvm_function_set_body(c, set, throw_set, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_string(args, key);
                    args[1] = thrown_object(get);
                    args[2] = thrown_object(set);
                    if (!(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (kind == 5)
                {
                    if (!(fvm_object_set_property_flags(c, object, key, 4, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                fvm_value_string(args, key);
                args[1] = thrown_object(watch);
                if (!(fvm_object_call(c, object, "watch", 2, args, &out) == FVM_OK && out.boolean))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                setter_calls = 0;
                fvm_value_number(&value, 81);
                FVM_Result status = fvm_object_set(c, object, key, &value);
                if (!version && kind == 5)
                {
                    if (!(status == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (kind == 4 && !setter_throws)
                {
                    if (!(status == FVM_OK && !c->exception_pending))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(status == FVM_EXCEPTION && fvm_exception_get(c, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (kind == 4)
                    {
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == 88))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(out.type == FVM_VALUE_OBJECT && out.object == watch_thrown))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_exception_clear(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(setter_calls == (version && (kind == 3 || kind == 4) ? 1U : 0U)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (kind == 3 || kind == 4)
                {
                    if (!(fvm_base_accessor_data(owner->accessors.find(fvm_detail::string(key))->second, out)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (kind == 3)
                    {
                        if (!(out.type == FVM_VALUE_UNDEFINED || (setter_throws && out.number == 9)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == 9))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                else
                {
                    if (!(fvm_object_get(c, object, key, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (version && kind != 5)
                    {
                        if (!(out.type == FVM_VALUE_UNDEFINED))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == 9))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
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
