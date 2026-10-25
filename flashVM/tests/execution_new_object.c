/* Named construction uses literal loaded names and skips loaded prototype getters while retaining arguments. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"

static unsigned called, getters;
static int fail;


static FVM_Result body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    FVM_Value value;
    (void)raw;
    if (!(frame->receiver && frame->argc == 1 && frame->argv[0].type == FVM_VALUE_OBJECT &&
          fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, frame->argv[0].object, "alive", &value) == FVM_OK &&
          value.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++called;
    if (fail)
    {
        fvm_value_number(&value, 77);
        return fvm_exception_raise(c, &value);
    }
    fvm_value_number(&value, 42);
    if (!(fvm_object_set(c, frame->receiver, "marker", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result prototype_getter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    (void)raw;
    ++getters;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    out->type = FVM_VALUE_OBJECT;
    out->object = frame->capture;
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned version;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (version = 0; version <= 8; ++version)
    {
        FVM_Context *c;
        FVM_Object *fn, *arg, *space, *get, *proto;
        FVM_Value value, name, args[3], out;
        int removed;
        const unsigned char code[] = {0};
        unsigned before = called, reads = getters;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (version && version < 4)
        {
            continue;
        }
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
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
        if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
              fvm_function_set_body(c, fn, body, 0, 0) == FVM_OK && fvm_object_create(c, &arg) == FVM_OK &&
              fvm_gc_pin(c, arg) == FVM_OK && fvm_object_create(c, &space) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, arg, "alive", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = space;
        if (!(fvm_set_global(c, "space", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = fn;
        if (!(fvm_set_global(c, "Ctor", &value) == FVM_OK && fvm_object_set(c, space, "Ctor", &value) == FVM_OK &&
              fvm_set_global(c, "space.Ctor", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(args);
        args[0].type = FVM_VALUE_OBJECT;
        args[0].object = arg;
        fvm_value_string(&name, "Ctor");
        if (!(fvm_exec_new_object(c, 0, 0, 0, 0, &name, 1, args, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT &&
              fvm_object_get(c, out.object, "marker", &value) == FVM_OK && value.number == 42 && called == before + 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "space.Ctor");
        if (!(fvm_exec_new_object(c, 0, 0, 0, 0, &name, 1, args, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "space:Ctor");
        fvm_value_number(&out, 99);
        if (!(fvm_exec_new_object(c, 0, 0, 0, 0, &name, 1, args, &out) == FVM_OK &&
              out.type == (version ? FVM_VALUE_UNDEFINED : FVM_VALUE_OBJECT)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            if (!(fvm_object_set_property_flags(c, fn, "prototype", 0, FVM_PROPERTY_DONT_DELETE) == FVM_OK &&
                  fvm_object_delete(c, fn, "prototype", &removed) == FVM_OK && removed &&
                  fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK &&
                  fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &get) == FVM_OK &&
                  fvm_function_set_body(c, get, prototype_getter, 0, proto) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(args, "prototype");
            fvm_value_undefined(args + 1);
            args[1].type = FVM_VALUE_OBJECT;
            args[1].object = get;
            fvm_value_undefined(args + 2);
            args[2].type = FVM_VALUE_NULL;
            if (!(fvm_object_call(c, fn, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, fn, "prototype", &value) == FVM_OK && value.object == proto &&
                  getters == reads + 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(args);
            args[0].type = FVM_VALUE_OBJECT;
            args[0].object = arg;
            fvm_value_string(&name, "Ctor");
            if (!(fvm_exec_new_object(c, 0, 0, 0, 0, &name, 1, args, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT &&
                  fvm_object_get(c, out.object, "marker", &value) == FVM_OK && value.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fail = 1;
            fvm_value_number(&out, 99);
            if (!(fvm_exec_new_object(c, 0, 0, 0, 0, &name, 1, args, &out) == FVM_EXCEPTION && out.number == 99 &&
                  fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(getters == reads + 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fail = 0;
            fvm_value_number(&value, 1);
            if (!(fvm_set_global(c, "Ctor", &value) == FVM_OK &&
                  fvm_exec_new_object(c, 0, 0, 0, 0, &name, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&name, "Missing");
            if (!(fvm_exec_new_object(c, 0, 0, 0, 0, &name, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_number(&out, 99);
        fvm_value_number(&name, 7);
        if (!(fvm_exec_new_object(c, 0, 0, 0, 0, &name, 0, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(getters == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
