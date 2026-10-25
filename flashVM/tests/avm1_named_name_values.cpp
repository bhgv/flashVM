/* Named actions convert names before counts, retaining callback-produced names through GC. */
#define main function2_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_function2.cpp"

#undef main
#include "flashvm/execution.h"
#include "flashvm/exception.h"

static unsigned order[4], calls;
static bool failure;


static FVM_Result name_body(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    order[calls++] = 1;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failure)
    {
        FVM_Value e;
        fvm_value_number(&e, 77);
        return fvm_exception_raise(c, &e);
    }
    return fvm_value_string(out, "17");
}


static FVM_Result count_body(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    order[calls++] = 2;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 0);
    return FVM_OK;
}


static FVM_Result target(FVM_Context *, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    order[calls++] = 3;
    if (!(frame->argc == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 42);
    return FVM_OK;
}


static FVM_Object *function(FVM_Context *c, FVM_ScriptBody body)
{
    const unsigned char code[] = {0};
    FVM_Object *fn = 0;
    if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
          fvm_function_set_body(c, fn, body, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fn;
}


static FVM_Object *operand(FVM_Context *c, const char *method, FVM_ScriptBody body)
{
    FVM_Object *o = 0;
    FVM_Value v;
    if (!(fvm_object_create(c, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = function(c, body);
    if (!(fvm_object_set(c, o, method, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return o;
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
        unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value v, out;
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = function(c, target);
        if (!(fvm_set_global(c, "17", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        v.object = operand(c, "toString", name_body);
        if (!(fvm_set_global(c, "name", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        v.object = operand(c, "valueOf", count_body);
        if (!(fvm_set_global(c, "count", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned action = 0; action < 2; ++action)
        {
            Bytes code;
            variable(code, "count");
            variable(code, "name");
            code.push_back(action ? 0x40 : 0x3d);
            code.push_back(0x3e);
            code.push_back(0);
            FVM_Object *fn = 0;
            if (!(fvm_function_create_avm1(c, code.begin(), code.size(), 0, 0, 0, &fn) == FVM_OK &&
                  fvm_gc_pin(c, fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            calls = 0;
            if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(calls == 3 && order[0] == 1 && order[1] == 2 && order[2] == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(action ? out.type == FVM_VALUE_OBJECT : out.number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            failure = true;
            calls = 0;
            if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_EXCEPTION && calls == 1 && order[0] == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            failure = false;
            fvm_value_number(&v, 17);
            if (!(fvm_set_global(c, "name", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            calls = 0;
            if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK && calls == 2 && order[0] == 2 && order[1] == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            v.type = FVM_VALUE_OBJECT;
            v.object = operand(c, "toString", name_body);
            if (!(fvm_set_global(c, "name", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
