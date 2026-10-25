/* C method calls use versioned stored names and retain a detached getter owner. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"
#include <cstring>

static FVM_Object *call_receiver, *call_owner, *call_function;
static unsigned call_fault, script_calls, getter_calls, native_calls;
static const char *call_stored;
static size_t call_size;


static FVM_Value call_object(FVM_Object *o)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = o;
    return value;
}


static FVM_Result script_method(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++script_calls;
    if (!(frame->receiver == call_receiver && c->calling_owner == call_owner))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame->argc == 1 && fvm_value_string_size(frame->argv) == 3 &&
          !std::memcmp(frame->argv[0].string, "a\0b", 3)))
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
        if (c->objects[i] == call_owner)
        {
            alive = true;
        }
    }
    if (!(alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, call_fault == 1 ? 73 : 17);
    return call_fault == 1 ? fvm_exception_raise(c, out) : FVM_OK;
}


static FVM_Result method_getter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++getter_calls;
    if (!(frame->receiver == call_receiver))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (call_fault == 2)
    {
        int removed = 0;
        if (!(fvm_object_set_prototype(c, call_receiver, 0) == FVM_OK &&
              fvm_object_delete_bytes(c, call_owner, call_stored, call_size, &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    *out = call_object(call_function);
    return FVM_OK;
}


static FVM_Result
native_method(FVM_Context *c, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out, void *)
{
    ++native_calls;
    if (!(receiver == call_receiver && n == 1 && fvm_value_string_size(args) == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, call_fault == 1 ? 73 : 29);
    return call_fault == 1 ? fvm_exception_raise(c, out) : FVM_OK;
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
        for (unsigned inherited = 0; inherited < 2; ++inherited)
        {
            for (unsigned kind = 0; kind < 4; ++kind)
            {
                for (unsigned spelling = 0; spelling < 2; ++spelling)
                {
                    for (call_fault = 0; call_fault < 3; ++call_fault)
                    {
                        if ((version && version < 4) || (call_fault == 2 && kind != 1))
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
                        FVM_Object *parent = 0, *get = 0;
                        if (!(fvm_construct(c, "Object", 0, 0, &call_receiver) == FVM_OK &&
                              fvm_gc_pin(c, call_receiver) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        call_owner = call_receiver;
                        if (inherited || kind == 2)
                        {
                            if (!(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK &&
                                  fvm_object_set_prototype(c, call_receiver, parent) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (inherited && kind != 2)
                            {
                                call_owner = parent;
                            }
                        }
                        call_stored = "MiXeD\0Name";
                        call_size = 10;
                        const unsigned char end[] = {0};
                        if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &call_function) == FVM_OK &&
                              fvm_function_set_body(c, call_function, script_method, 0, 0) == FVM_OK &&
                              fvm_gc_pin(c, call_function) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Value value = call_object(call_function), args[3], out;
                        if (kind == 2)
                        {
                            if (!(fvm_object_define_method_rooted_bytes(
                                      c, parent, call_stored, call_size, native_method, 0, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind == 3)
                        {
                            if (!(fvm_object_define_method_rooted_bytes(
                                      c, call_owner, call_stored, call_size, native_method, 0, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (kind == 1)
                        {
                            if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK &&
                                  fvm_function_set_body(c, get, method_getter, 0, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            fvm_value_string_bytes(args, call_stored, call_size);
                            args[1] = call_object(get);
                            fvm_value_undefined(args + 2);
                            args[2].type = FVM_VALUE_NULL;
                            if (!(fvm_object_call(c, call_owner, "addProperty", 3, args, &out) == FVM_OK &&
                                  out.boolean))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(fvm_object_set_bytes(c, call_owner, call_stored, call_size, &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        script_calls = getter_calls = native_calls = 0;
                        fvm_value_string_bytes(&value, "a\0b", 3);
                        FVM_Result status = fvm_object_call_bytes(
                            c, call_receiver, spelling ? "mixed\0name" : call_stored, call_size, 1, &value, &out);
                        bool matched = !spelling || (version && version <= 6);
                        if (!(status == (!matched ? FVM_UNSUPPORTED : call_fault == 1 ? FVM_EXCEPTION : FVM_OK)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(script_calls == (matched && kind != 3 ? 1U : 0U) &&
                              native_calls == (matched && kind == 3 ? 1U : 0U) &&
                              getter_calls == (matched && kind == 1 ? 1U : 0U)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (status == FVM_OK)
                        {
                            if (!(out.type == FVM_VALUE_NUMBER && out.number == (kind == 3 ? 29 : 17)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (status == FVM_EXCEPTION)
                        {
                            if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == 73 &&
                                  fvm_exception_clear(c) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_gc_collect(c, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_context_destroy(c);
                    }
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
