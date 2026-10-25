/* Anonymous CallMethod depth and capture roots survive removal of the function's last property. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_adapter_super.cpp"
#include "../src/core/call_scope.hpp"
#include "../src/standard/super.hpp"
#include "flashvm/exception.h"

static FVM_Object *function, *capture;
static unsigned mode;


static FVM_Result body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(frame->receiver == c->global && c->calling_function == function && !c->calling_owner &&
          !c->calling_constructor && c->calling_super_depth == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!frame->implicit_this && !c->script_this && c->script_this_undefined && c->calling_this_undefined))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = 0;
    if (!(fvm_object_delete(c, c->global, "anonymousFunction", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set_prototype(c, c->global, 0) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool alive = false;
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        alive |= c->objects[i] == capture;
    }
    if (!(alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (mode == 1)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    if (mode == 2)
    {
        return FVM_ERROR;
    }
    return fvm_super_create(c, function, c->global, out);
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
        for (mode = 0; mode < 3; ++mode)
        {
            for (unsigned route = 0; route < 4; ++route)
            {
                if (route >= 2 && !fvm_jit_available())
                {
                    continue;
                }
                FVM_Context *c = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                version_load(c, version);
                if (!(fvm_standard_install(c) == FVM_OK && fvm_construct(c, "Object", 0, 0, &capture) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                function = adapter_super_function(c, 0);
                if (!(fvm_function_set_body(c, function, body, 0, capture) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value value = class_value(function), out, name;
                if (!(fvm_set_global(c, "anonymousFunction", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_string(&name, "");
                const FVM_Result expected = mode == 1 ? FVM_EXCEPTION : mode == 2 ? FVM_ERROR : FVM_OK;
                {


                    FVM_CallScope outer(c, 0, 0, 0, 7);
                    if (!route)
                    {
                        if (!(fvm_exec_call_method(c, 0, &value, &name, c->global, 0, 0, &out) == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (mode == 1)
                        {
                            if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == 77))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_exception_clear(c) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    else
                    {
                        Program code;
                        code.number(0);
                        code.variable("anonymousFunction");
                        code.text("");
                        code.op(0x52);
                        code.finish();
                        out = boxing_run(c, code, route - 1, expected);
                    }
                    if (!(c->calling_super_depth == 7))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!mode)
                {
                    if (!(out.type == FVM_VALUE_UNDEFINED))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(!c->calling_this_undefined && !c->script_this_undefined && !c->script_this && !c->gc_frames &&
                      !c->gc_native_depth && !c->calling_function && !c->calling_owner && !c->calling_constructor &&
                      c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH && !c->exception_pending))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    /* Unloaded host inference still has no super for a function without a defining owner. */
    FVM_Context *c = 0;
    if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value owner = class_value(adapter_super_function(c, 0)), name, out;
    fvm_value_string(&name, "");
    if (!(fvm_exec_call_method(c, 0, &owner, &name, c->global, 0, 0, &out) == FVM_OK &&
          out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
