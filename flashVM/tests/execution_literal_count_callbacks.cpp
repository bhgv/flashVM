/* Direct C count conversion roots an otherwise unreachable receiver through collecting coercion. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/runtime.hpp"

static unsigned calls;
static bool fail;


static FVM_Result count(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++calls;
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value v;
    if (!(fvm_object_get(c, frame->receiver, "alive", &v) == FVM_OK && v.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 1.9);
    return fail ? fvm_exception_raise(c, out) : FVM_OK;
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
        for (unsigned failing = 0; failing < 2; ++failing)
        {
            FVM_Context *c = 0;
            FVM_Object *object = 0, *fn = 0;
            FVM_Value v, input;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            const unsigned char code[] = {0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
                  fvm_standard_install(c) == FVM_OK && fvm_object_create(c, &object) == FVM_OK &&
                  fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
                  fvm_function_set_body(c, fn, count, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&v);
            v.type = FVM_VALUE_OBJECT;
            v.object = fn;
            if (!(fvm_object_set(c, object, "valueOf", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&v, 17);
            if (!(fvm_object_set(c, object, "alive", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&input);
            input.type = FVM_VALUE_OBJECT;
            input.object = object;
            FVM_GCRootFrame *roots = c->gc_frames;
            calls = 0;
            fail = failing != 0;
            FVM_Result r = fvm_exec_literal_count(c, &input, &input);
            if (!(calls == 1 && c->gc_frames == roots && !c->gc_native_depth && !c->script_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (failing)
            {
                if (!(r == FVM_EXCEPTION && input.type == FVM_VALUE_OBJECT && input.object == object &&
                      fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(r == FVM_OK && input.type == FVM_VALUE_NUMBER && input.number == 1))
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
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
