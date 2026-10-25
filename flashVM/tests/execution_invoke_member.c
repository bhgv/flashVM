/* C89 direct named calls distinguish missing/scalar/noncallable members from callable values. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"

static FVM_Object *expected;
static unsigned calls;


static FVM_Result
body(FVM_Context *c, FVM_Object *receiver, unsigned argc, const FVM_Value *argv, FVM_Value *out, void *raw)
{
    (void)c;
    (void)raw;
    if (!(receiver == expected && argc == 1 && argv[0].number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++calls;
    fvm_value_number(out, 42);
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
        FVM_Object *o, *fn, *plain;
        FVM_Value owner, key, value, arg, out;
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
        if (!(fvm_standard_install(c) == FVM_OK && fvm_object_create(c, &o) == FVM_OK &&
              fvm_object_create(c, &plain) == FVM_OK && fvm_function_create_native(c, body, 0, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        expected = o;
        fvm_value_undefined(&owner);
        owner.type = FVM_VALUE_OBJECT;
        owner.object = o;
        fvm_value_string(&key, "cell");
        fvm_value_number(&arg, 17);
        if (!(fvm_exec_invoke_member(c, o, &key, 1, &arg, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 99);
        if (!(fvm_object_set(c, o, "cell", &value) == FVM_OK &&
              fvm_exec_invoke_member(c, o, &key, 1, &arg, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.type = FVM_VALUE_OBJECT;
        value.object = plain;
        if (!(fvm_object_set(c, o, "cell", &value) == FVM_OK &&
              fvm_exec_invoke_member(c, o, &key, 1, &arg, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = fn;
        if (!(fvm_object_set_bytes(c, o, "call\0x", 6, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string_bytes(&key, "call\0x", 6);
        if (!(fvm_exec_invoke_member(c, o, &key, 1, &arg, &out) == FVM_OK && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&out, 99);
        if (!(fvm_exec_invoke_member(c, 0, &key, 1, &arg, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            fvm_value_string(&key, "missing");
            if (!(fvm_exec_call_method(c, 0, &owner, &key, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&value);
            if (!(fvm_set_global(c, "Number", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&owner, 42);
            if (!(fvm_exec_call_method(c, 0, &owner, &key, 0, 0, 0, &out) == FVM_OK &&
                  out.type == FVM_VALUE_UNDEFINED &&
                  fvm_exec_call_method(c, 1, &owner, &key, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&key, "");
            if (!(fvm_exec_call_method(c, 0, &owner, &key, 0, 0, 0, &out) == FVM_OK &&
                  out.type == FVM_VALUE_UNDEFINED &&
                  fvm_exec_call_method(c, 1, &owner, &key, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    if (!(calls == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
