/* Resolver-returned methods preserve capture roots and fixed depth after resolver deletion. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_adapter_super.cpp"
#include "../src/standard/super.hpp"
#include "../src/core/call_scope.hpp"
#include "flashvm/exception.h"

static FVM_Object *receiver, *method, *capture, *resolver_owner;
static unsigned expected_depth;
static unsigned mode;


static FVM_Result method_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(frame->receiver == receiver && c->calling_function == method && !c->calling_owner &&
          c->calling_super_depth == expected_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
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
    return fvm_super_create(c, method, receiver, out);
}


static FVM_Result resolver_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(c->calling_super_depth == 0 && frame->receiver == receiver))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = 0;
    if (!(fvm_object_delete(c, resolver_owner, "__resolve", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set_prototype(c, receiver, 0) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    *out = class_value(frame->capture);
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
        for (unsigned parent_call = 0; parent_call < 2; ++parent_call)
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
                    if (!(fvm_standard_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK &&
                          fvm_construct(c, "Object", 0, 0, &capture) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    method = adapter_super_function(c, 0);
                    if (!(fvm_function_set_body(c, method, method_body, 0, capture) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *resolver = adapter_super_function(c, 0);
                    if (!(fvm_function_set_body(c, resolver, resolver_body, 0, method) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *parent = 0;
                    if (!(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK &&
                          fvm_construct(c, "Object", 0, 0, &resolver_owner) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_set_prototype(c, receiver, parent) == FVM_OK &&
                          fvm_object_set_prototype(c, parent, resolver_owner) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value value = class_value(resolver), out;
                    if (!(fvm_object_set(c, resolver_owner, "__resolve", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *target = receiver;
                    expected_depth = parent_call + 1;
                    if (parent_call)
                    {


                        FVM_CallScope scope(c, 0, method, 0, 1);
                        if (!(fvm_super_create(c, method, receiver, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        target = out.object;
                    }
                    value = class_value(target);
                    if (!(fvm_set_global(c, "returnedRootTarget", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    const FVM_Result expected = mode == 1 ? FVM_EXCEPTION : mode == 2 ? FVM_ERROR : FVM_OK;
                    {


                        FVM_CallScope outer(c, 0, 0, 0, 7);
                        if (!route)
                        {
                            if (!(fvm_object_call(c, target, "missing", 0, 0, &out) == expected))
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
                            code.variable("returnedRootTarget");
                            code.text("missing");
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
                        if (!(out.type == FVM_VALUE_OBJECT &&
                              out.object->resource_result.object->properties["depth"].number == expected_depth))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!c->resolve_depth && !c->gc_frames && !c->gc_native_depth && !c->calling_function &&
                          !c->calling_owner && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH &&
                          !c->exception_pending))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
