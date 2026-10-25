#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/avm1.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include <string.h>

static unsigned calls;


static FVM_Result
sink(FVM_Context *c, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    (void)c;
    (void)receiver;
    (void)raw;
    if (n == 3)
    {
        if (!(args[0].string_size == 3 && !memcmp(args[0].string, "a\0b", 3)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(args[2].number == 194))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(n == 1 && !strcmp(args[0].string, "undefined")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    ++calls;
    fvm_value_undefined(out);
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Object *o;
    FVM_Value a, b, v, key, owner;
    unsigned version;
    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (version = 4; version <= 8; ++version)
    {
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&a, 10);
        fvm_value_number(&b, 3);
        if (!(fvm_exec_numeric(c, FVM_EXEC_SUBTRACT, &a, &b, &a) == FVM_OK && a.number == 7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_numeric(c, FVM_EXEC_REMAINDER, &a, &b, &v) == FVM_OK && v.number == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&a, -1);
        fvm_value_number(&b, 0);
        if (!(fvm_exec_numeric(c, FVM_EXEC_SHIFT_UNSIGNED, &a, &b, &v) == FVM_OK &&
              v.number == (version == 8 ? -1 : 4294967295.0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&a, 1);
        if (!(fvm_exec_numeric(c, FVM_EXEC_DIVIDE, &a, &b, &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(version == 4 ? v.type == FVM_VALUE_STRING && !strcmp(v.string, "#ERROR#")
                           : v.type == FVM_VALUE_NUMBER && v.number > 1e300))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&a, "x");
        fvm_value_number(&b, 2);
        if (!(fvm_exec_compare(c, FVM_EXEC_ADD_DYNAMIC, &a, &b, &a) == FVM_OK && !strcmp(a.string, "x2")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_compare(c, FVM_EXEC_STRICT_EQUAL, &a, &b, &v) == FVM_OK && !v.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&owner);
        owner.type = FVM_VALUE_OBJECT;
        owner.object = o;
        fvm_value_string(&key, "k");
        fvm_value_number(&a, 42);
        if (!(fvm_exec_member(c, 1, &owner, &key, &a, &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_member(c, 0, &owner, &key, 0, &v) == FVM_OK && v.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 99);
        if (!(fvm_exec_numeric(c, (FVM_ExecutionNumeric)99, &a, &b, &v) == FVM_INVALID_ARGUMENT && v.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_numeric(c, FVM_EXEC_ADD, 0, &b, &v) == FVM_INVALID_ARGUMENT && v.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_member(c, 0, &owner, 0, 0, &v) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_avm1_set_url_rooted(c, sink, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string_bytes(&a, "a\0b", 3);
        fvm_value_string(&b, "_blank");
        if (!(fvm_exec_url(c, &a, &b, 194) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_url(c, &a, &b, 3) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_avm1_set_trace_rooted(c, sink, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&a);
        if (!(fvm_exec_trace(c, &a) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(calls == 10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
