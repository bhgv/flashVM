/* A getter may detach its defining prototype and collect before returning a method. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"

static unsigned reads, invocations;
static bool fail_read;


static FVM_Result method_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    FVM_Value v;
    if (!(fvm_gc_collect(c, 0) == FVM_OK && c->calling_owner &&
          fvm_object_get(c, c->calling_owner, "alive", &v) == FVM_OK && v.number == 17 && frame->argc == 1 &&
          frame->argv[0].type == FVM_VALUE_OBJECT && fvm_object_get(c, frame->argv[0].object, "alive", &v) == FVM_OK &&
          v.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++invocations;
    fvm_value_number(out, 42);
    return FVM_OK;
}


static FVM_Result read_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++reads;
    if (!(fvm_object_set_prototype(c, frame->receiver, 0) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (fail_read)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = frame->capture;
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
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned throwing = 0; throwing < 2; ++throwing)
        {
            FVM_Object *receiver = 0, *proto = 0, *method = 0, *getter = 0, *argument = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK &&
                  fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK &&
                  fvm_object_set_prototype(c, receiver, proto) == FVM_OK && fvm_object_create(c, &argument) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value owner, key, value, args[3], out;
            fvm_value_number(&value, 17);
            if (!(fvm_object_set(c, proto, "alive", &value) == FVM_OK &&
                  fvm_object_set(c, argument, "alive", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            const unsigned char code[] = {0};
            if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &method) == FVM_OK &&
                  fvm_function_set_body(c, method, method_body, 0, 0) == FVM_OK &&
                  fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &getter) == FVM_OK &&
                  fvm_function_set_body(c, getter, read_body, 0, method) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(args, "cell");
            fvm_value_undefined(args + 1);
            args[1].type = FVM_VALUE_OBJECT;
            args[1].object = getter;
            fvm_value_undefined(args + 2);
            args[2].type = FVM_VALUE_NULL;
            if (!(fvm_object_call(c, proto, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&owner);
            owner.type = FVM_VALUE_OBJECT;
            owner.object = receiver;
            value.type = FVM_VALUE_OBJECT;
            value.object = argument;
            fvm_value_string(&key, "cell");
            fail_read = throwing != 0;
            fvm_value_number(&out, 99);
            FVM_Result status = fvm_exec_call_method(c, 0, &owner, &key, 0, 1, &value, &out);
            if (!(c->gc_frames == 0 && !c->calling_owner))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (throwing)
            {
                if (!(status == FVM_EXCEPTION && out.number == 99 && fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(status == FVM_OK && out.number == 42))
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
        }
        fvm_context_destroy(c);
    }
    if (!(reads == 10 && invocations == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
