/* Script setters may collect; receiver/owner/descriptor and incoming values remain retained. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/property.h"
#include "flashvm/exception.h"
#include <string.h>

static int mode, in_host, do_collect = 1;
static unsigned setter_entries;


static FVM_Result setter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    FVM_Value value;
    int deleted;
    (void)raw;
    ++setter_entries;
    if (!(frame->argc == 1 && frame->receiver && frame->capture))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!do_collect)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    if (mode == 1)
    {
        if (!(fvm_object_delete(c, frame->receiver, "value", &deleted) == FVM_OK && deleted))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (mode == 3)
    {
        if (!(fvm_object_set_prototype(c, frame->receiver, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (do_collect)
    {
        if (!(fvm_gc_collect(c, 0) == (in_host ? FVM_INVALID_ARGUMENT : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_object_get(c, frame->capture, "alive", &value) == FVM_OK && value.number == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (frame->argv[0].type == FVM_VALUE_OBJECT)
    {
        if (!(fvm_object_get(c, frame->argv[0].object, "alive", &value) == FVM_OK && value.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (mode == 2)
    {
        fvm_value_number(&value, 77);
        return fvm_exception_raise(c, &value);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result host(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    FVM_Result r;
    (void)raw;
    if (!(n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    in_host = 1;
    r = fvm_object_set(c, self, "value", args);
    in_host = 0;
    fvm_value_undefined(out);
    return r;
}


static FVM_Result
native_setter(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    (void)self;
    (void)n;
    (void)args;
    (void)raw;
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static void install(FVM_Context *c, FVM_Object *owner, FVM_Object *plain, FVM_Object *fn)
{
    FVM_Value args[3], out;
    fvm_value_string(args, "value");
    fvm_value_undefined(args + 1);
    args[1].type = FVM_VALUE_OBJECT;
    args[1].object = plain;
    fvm_value_undefined(args + 2);
    args[2].type = FVM_VALUE_OBJECT;
    args[2].object = fn;
    if (!(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_SETTER_COLLECTOR_FIXTURE_ONLY


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Object *owner, *plain, *capture, *fn, *input, *parent, *child, *host_fn;
    FVM_Value value, out;
    const unsigned char code[] = {0};
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK && fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK &&
          fvm_object_create(c, &capture) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 99);
    if (!(fvm_object_set(c, capture, "alive", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
          fvm_function_set_body(c, fn, setter, 0, capture) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 11);
    if (!(fvm_object_set(c, owner, "value", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    install(c, owner, plain, fn);
    fvm_value_number(&value, 17);
    if (!(fvm_object_set(c, owner, "value", &value) == FVM_OK && fvm_object_get(c, owner, "value", &out) == FVM_OK &&
          out.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    mode = 2;
    if (!(fvm_object_set(c, owner, "value", &value) == FVM_EXCEPTION && fvm_exception_clear(c) == FVM_OK &&
          fvm_object_get(c, owner, "value", &out) == FVM_OK && out.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    mode = 0;
    fvm_value_string(&value, "borrowed");
    if (!(fvm_object_set(c, owner, "value", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, owner, "value", &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
          !strcmp(out.string, "borrowed")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    mode = 1;
    if (!(fvm_object_create(c, &input) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 42);
    if (!(fvm_object_set(c, input, "alive", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = input;
    if (!(fvm_object_set(c, owner, "value", &value) == FVM_OK && fvm_object_get(c, owner, "value", &out) == FVM_OK &&
          out.object == input))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    mode = 0;
    install(c, owner, plain, fn);
    if (!(fvm_function_create_native(c, host, 0, 0, &host_fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 17);
    if (!(fvm_function_invoke(c, host_fn, owner, 1, &value, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_accessor_rooted(c, owner, "native", 0, native_setter, 0, 0) == FVM_OK &&
          fvm_object_set(c, owner, "native", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK && fvm_construct(c, "Object", 0, 0, &child) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 11);
    if (!(fvm_object_set(c, parent, "value", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    install(c, parent, plain, fn);
    if (!(fvm_object_set_prototype(c, child, parent) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    mode = 3;
    fvm_value_number(&value, 42);
    if (!(fvm_object_set(c, child, "value", &value) == FVM_OK && fvm_object_get(c, parent, "value", &out) == FVM_OK &&
          out.number == 11))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, child, "value", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED && setter_entries == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
