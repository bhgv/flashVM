/* Explicit-super constructor depth survives getter deletion, missing parents and collection. */
#define FVM_PROPERTY_CALLBACK_SUPER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "property_callback_super.cpp"
#include "../src/core/call_scope.hpp"
#include "../src/standard/super.hpp"
#include "flashvm/exception.h"

static FVM_Object *receiver, *owner, *method, *capture, *selected_parent;
static unsigned expected_depth, mode;


static FVM_Result getter_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(frame->receiver == owner))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = 0;
    if (!(fvm_object_delete(c, owner, "__constructor__", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set_prototype(c, receiver, 0) == FVM_OK && fvm_object_set_prototype(c, owner, 0) == FVM_OK &&
          fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    *out = class_value(frame->capture);
    return FVM_OK;
}


static FVM_Result method_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(frame->receiver == receiver && c->calling_owner == selected_parent && c->calling_constructor == method &&
          c->calling_function == method && c->calling_super_depth == expected_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool owner_live = false, capture_live = false, parent_live = false;
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        parent_live |= c->objects[i] == selected_parent;
        owner_live |= c->objects[i] == owner;
        capture_live |= c->objects[i] == capture;
    }
    if (!(owner_live && capture_live && parent_live))
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
        for (unsigned base = 0; base < 3; ++base)
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
                    FVM_Object *nodes[7];
                    for (unsigned i = 0; i < 7; ++i)
                    {
                        if (!(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    receiver = nodes[0];
                    owner = nodes[base];
                    expected_depth = base + 1;
                    if (!(fvm_gc_pin(c, receiver) == FVM_OK && fvm_construct(c, "Object", 0, 0, &capture) == FVM_OK))
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
                    FVM_Object *getter = adapter_super_function(c, 0);
                    if (!(fvm_function_set_body(c, getter, getter_body, 0, method) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_set_property_flags(c, owner, "__constructor__", 0, 4) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    callback_super_property(c, owner, "__constructor__", 15, getter, version_function(c, 0));
                    for (unsigned i = 0; i < 7; ++i)
                    {
                        if (!(fvm_object_set_prototype(c, nodes[i], i < 6 ? nodes[i + 1] : 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    selected_parent = owner->prototype;
                    FVM_Value value, out;
                    {


                        FVM_CallScope scope(c, 0, method, 0, base);
                        if (!(fvm_super_create(c, method, receiver, &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    FVM_Object *target = value.object;
                    if (!(fvm_set_global(c, "capturedMemberTarget", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Result expected = mode == 1 ? FVM_EXCEPTION : mode == 2 ? FVM_ERROR : FVM_OK;
                    {


                        FVM_CallScope outer(c, 0, 0, 0, 7);
                        if (!route)
                        {
                            if (!(fvm_function_invoke(c, target, receiver, 0, 0, &out) == expected))
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
                            code.variable("capturedMemberTarget");
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
                        if (!(out.type == FVM_VALUE_OBJECT &&
                              out.object->resource_result.object->properties["depth"].number == expected_depth))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner &&
                          !c->calling_setter_owner && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH &&
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
