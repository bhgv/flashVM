/* C89 qualified DefineLocal2 writes existing undefined values, including accessors. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/exception.h"
#include "flashvm/gc.h"
#include "flashvm/function.h"
#include "flashvm/movieclip.h"

static unsigned getter_calls; /* Getter invocations observed by this fixture. */
static unsigned setter_calls; /* Setter invocations observed by this fixture. */
static int defined, fail;


static FVM_Result getter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    (void)frame;
    (void)raw;
    ++getter_calls;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (fail)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    if (defined == 2)
    {
        return fvm_get_global(c, "clip", out);
    }
    if (defined)
    {
        fvm_value_number(out, 17);
    }
    else
    {
        fvm_value_undefined(out);
    }
    return FVM_OK;
}


static FVM_Result setter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    (void)c;
    (void)raw;
    if (!(frame->argc == 1 && frame->argv[0].type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++setter_calls;
    fvm_value_undefined(out);
    return FVM_OK;
}


static void accessors(FVM_Context *c, FVM_Object *holder)
{
    FVM_Object *get, *put;
    FVM_Value args[3], out;
    const unsigned char code[] = {0};
    if (!(fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &get) == FVM_OK &&
          fvm_function_set_body(c, get, getter, 0, 0) == FVM_OK &&
          fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &put) == FVM_OK &&
          fvm_function_set_body(c, put, setter, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(args, "x");
    fvm_value_undefined(args + 1);
    args[1].type = FVM_VALUE_OBJECT;
    args[1].object = get;
    fvm_value_undefined(args + 2);
    args[2].type = FVM_VALUE_OBJECT;
    args[2].object = put;
    if (!(fvm_object_call(c, holder, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned v;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (v = 4; v <= 8; ++v)
    {
        FVM_Context *c;
        FVM_Object *holder, *clip;
        FVM_Value name, value;
        unsigned before;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)v;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK && fvm_construct(c, "Object", 0, 0, &holder) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = holder;
        if (!(fvm_set_global(c, "holder", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = clip;
        if (!(fvm_set_global(c, "clip", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        accessors(c, holder);
        fvm_value_string(&name, "holder:x");
        defined = 0;
        before = setter_calls;
        if (!(fvm_exec_variable_declare(c, 0, 0, 0, 0, FVM_EXEC_DECLARE_UNINITIALIZED, &name, 0) == FVM_OK &&
              setter_calls == before + 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        defined = 1;
        before = setter_calls;
        if (!(fvm_exec_variable_declare(c, 0, 0, 0, 0, FVM_EXEC_DECLARE_UNINITIALIZED, &name, 0) == FVM_OK &&
              setter_calls == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail = 1;
        if (!(fvm_exec_variable_declare(c, 0, 0, 0, 0, FVM_EXEC_DECLARE_UNINITIALIZED, &name, 0) == FVM_EXCEPTION &&
              setter_calls == before && fvm_exception_clear(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail = 0;
        defined = 2;
        before = setter_calls;
        if (!(fvm_exec_variable_declare(c, 0, 0, 0, 0, FVM_EXEC_DECLARE_UNINITIALIZED, &name, 0) == FVM_OK &&
              setter_calls == before + (v == 4 ? 1U : 0U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(getter_calls == 20 && setter_calls == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
