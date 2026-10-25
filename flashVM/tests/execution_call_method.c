/* Direct C89 calls exercise the shared method service without bytecode. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/standard.h"
#include <string.h>

static FVM_Object *expected;
static unsigned calls;


static FVM_Result method(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    (void)c;
    (void)raw;
    if (!(self == expected && n == 1 && args[0].number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) != FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++calls;
    fvm_value_number(out, 23);
    return FVM_OK;
}


static FVM_Result body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    (void)raw;
    if (!(frame->receiver && frame->argc == 1 && frame->argv[0].number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 31);
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Object *o, *fn;
    FVM_Value owner, name, arg, out, callable;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_native(c, method, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&owner);
    owner.type = FVM_VALUE_OBJECT;
    owner.object = o;
    fvm_value_undefined(&callable);
    callable.type = FVM_VALUE_OBJECT;
    callable.object = fn;
    if (!(fvm_object_set_bytes(c, o, "m\0x", 3, &callable) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string_bytes(&name, "m\0x", 3);
    fvm_value_number(&arg, 17);
    expected = o;
    if (!(fvm_exec_call_method(c, 0, &owner, &name, 0, 1, &arg, &out) == FVM_OK && out.number == 23))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&name, "");
    if (!(fvm_exec_call_method(c, 0, &callable, &name, o, 1, &arg, &callable) == FVM_OK && callable.number == 23))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(calls == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&owner, "abc");
    fvm_value_string(&name, "charAt");
    fvm_value_number(&arg, 1);
    if (!(fvm_exec_call_method(c, 0, &owner, &name, 0, 1, &arg, &owner) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(owner.type == FVM_VALUE_STRING && !strcmp(owner.string, "b")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&owner);
    owner.type = FVM_VALUE_NULL;
    fvm_value_number(&out, 99);
    if (!(fvm_exec_call_method(c, 1, &owner, &name, 0, 0, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_call_method(c, 0, &owner, &name, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&name, 1);
    fvm_value_number(&out, 99);
    if (!(fvm_exec_call_method(c, 0, &owner, &name, 0, 0, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        const unsigned char code[] = {0};
        if (!(fvm_object_create(c, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, fn, body, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&callable);
        callable.type = FVM_VALUE_OBJECT;
        callable.object = fn;
        if (!(fvm_object_set_bytes(c, o, "run", 3, &callable) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&owner);
        owner.type = FVM_VALUE_OBJECT;
        owner.object = o;
        fvm_value_string(&name, "run");
        fvm_value_number(&arg, 17);
        if (!(fvm_exec_call_method(c, 0, &owner, &name, 0, 1, &arg, &out) == FVM_OK && out.number == 31))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_call_method(c, 1, &owner, &name, 0, 1, &arg, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
