/* Undefined this survives nested named/anonymous calls, collection and exceptional exits. */
#define FVM_UNDEFINED_THIS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "anonymous_undefined_this.cpp"
#include "../src/core/call_scope.hpp"
#include "flashvm/exception.h"

static FVM_Object *parent_function, *named_function, *inherited_function, *captured;
static unsigned failure_mode;


static FVM_Result nested_this(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(!c->calling_this_undefined && !c->script_this_undefined))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame->implicit_this == c->global && c->script_this == c->global))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 29);
    return FVM_OK;
}


static FVM_Result undefined_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(frame->receiver == c->global && !frame->implicit_this))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->calling_this_undefined && c->script_this_undefined && !c->script_this))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value owner = class_value(c->global), name;
    fvm_value_string(&name, "namedChild");
    if (!(fvm_exec_call_method(c, 0, &owner, &name, c->global, 0, 0, out) == FVM_OK && out->number == 29))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->calling_this_undefined && c->script_this_undefined && !c->script_this &&
          c->calling_function == parent_function))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (inherited_function)
    {
        owner = class_value(inherited_function);
        fvm_value_string(&name, "");
        if (!(fvm_exec_call_method(c, 0, &owner, &name, c->global, 0, 0, out) == FVM_OK &&
              out->type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->calling_this_undefined && c->script_this_undefined && !c->script_this))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, c->global, "namedInherited", 0, 0, out) == FVM_OK && out->type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, c->global, "namedPreload", 0, 0, out) == FVM_OK && out->type == FVM_VALUE_OBJECT &&
              out->object == c->global))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, c->global, "namedSuper", 0, 0, out) == FVM_OK && out->type == FVM_VALUE_OBJECT &&
              out->object->resource_kind == FVM_RESOURCE_SUPER))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->calling_this_undefined && c->script_this_undefined && !c->script_this))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
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
        alive |= c->objects[i] == captured;
    }
    if (!(alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failure_mode == 1)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    if (failure_mode == 2)
    {
        return FVM_ERROR;
    }
    fvm_value_undefined(out);
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
        for (failure_mode = 0; failure_mode < 3; ++failure_mode)
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
                if (!(fvm_standard_install(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                inherited_function = version >= 7 ? undefined_this_function(c, 5) : 0;
                if (inherited_function)
                {
                    FVM_Value child = class_value(inherited_function);
                    if (!(fvm_set_global(c, "namedInherited", &child) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    child = class_value(undefined_this_function(c, 2));
                    if (!(fvm_set_global(c, "namedPreload", &child) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    child = class_value(undefined_this_function(c, 6));
                    if (!(fvm_set_global(c, "namedSuper", &child) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                named_function = undefined_this_function(c, 0);
                if (!(fvm_function_set_body(c, named_function, nested_this, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value value = class_value(named_function);
                if (!(fvm_set_global(c, "namedChild", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_construct(c, "Object", 0, 0, &captured) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                parent_function = undefined_this_function(c, 0);
                if (!(fvm_function_set_body(c, parent_function, undefined_body, 0, captured) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                value = class_value(parent_function);
                if (!(fvm_set_global(c, "anonymousFunction", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const FVM_Result expected = failure_mode == 1 ? FVM_EXCEPTION : failure_mode == 2 ? FVM_ERROR : FVM_OK;
                FVM_Value name, out;
                fvm_value_string(&name, "");
                {


                    FVM_CallScope outer(c, 0, 0, 0, 7);
                    if (!route)
                    {
                        if (!(fvm_exec_call_method(c, 0, &value, &name, c->global, 0, 0, &out) == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (failure_mode == 1)
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
                    if (!(c->calling_super_depth == 7 && !c->calling_this_undefined && !c->script_this_undefined))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(!c->gc_frames && !c->script_depth && !c->script_this && !c->calling_this_undefined &&
                      !c->script_this_undefined && !c->exception_pending))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
