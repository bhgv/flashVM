/* Fixed property depth retains the separate descriptor owner through split chains, GC and failures. */
#define FVM_PROPERTY_CALLBACK_SUPER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "property_callback_super.cpp"
#include "../src/standard/super.hpp"
#include "../src/core/call_scope_constants.h"
#include "flashvm/exception.h"

static FVM_Object *property_depth_receiver, *property_depth_first, *property_depth_owner, *property_depth_capture,
    *property_depth_function;
static unsigned property_depth_mode;
static bool property_depth_setter;


static FVM_Result property_depth_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    if (!(frame->receiver == property_depth_receiver && c->calling_function == property_depth_function &&
          c->calling_owner == property_depth_first && c->calling_super_depth == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->calling_setter_owner == (property_depth_setter ? property_depth_owner : 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (property_depth_mode == 3)
    {
        int removed = 0;
        if (!(fvm_object_delete(c, property_depth_owner, "slot", &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_prototype(c, property_depth_receiver, 0) == FVM_OK &&
              fvm_object_set_prototype(c, property_depth_first, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool owner_alive = false, capture_alive = false;
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        owner_alive |= c->objects[i] == property_depth_owner;
        capture_alive |= c->objects[i] == property_depth_capture;
    }
    if (!(owner_alive && capture_alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (property_depth_mode == 1)
    {
        fvm_value_number(out, 77);
        return fvm_exception_raise(c, out);
    }
    if (property_depth_mode == 2)
    {
        return FVM_ERROR;
    }
    if (!(fvm_super_create(c, property_depth_function, frame->receiver, out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (property_depth_setter)
    {
        if (!(fvm_object_set(c, frame->receiver, "savedSuper", out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
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
        for (unsigned setter = 0; setter < 2; ++setter)
        {
            for (property_depth_mode = 0; property_depth_mode < 4; ++property_depth_mode)
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
                    if (!(fvm_construct(c, "Object", 0, 0, &property_depth_receiver) == FVM_OK &&
                          fvm_gc_pin(c, property_depth_receiver) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_construct(c, "Object", 0, 0, &property_depth_first) == FVM_OK &&
                          fvm_construct(c, "Object", 0, 0, &property_depth_owner) == FVM_OK &&
                          fvm_construct(c, "Object", 0, 0, &property_depth_capture) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    property_depth_function = adapter_super_function(c, 0);
                    if (!(fvm_function_set_body(
                              c, property_depth_function, property_depth_body, 0, property_depth_capture) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    callback_super_property(
                        c, property_depth_owner, "slot", 4, property_depth_function, property_depth_function);
                    if (!(fvm_object_set_prototype(c, property_depth_first, property_depth_owner) == FVM_OK &&
                          fvm_object_set_prototype(c, property_depth_receiver, property_depth_first) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    property_depth_setter = setter != 0;
                    FVM_Value value = class_value(property_depth_receiver), out;
                    if (!(fvm_set_global(c, "depthPropertyTarget", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    const FVM_Result expected = property_depth_mode == 1   ? FVM_EXCEPTION
                                                : property_depth_mode == 2 ? FVM_ERROR
                                                                           : FVM_OK;
                    if (!route)
                    {
                        if (setter)
                        {
                            fvm_value_number(&value, 81);
                            if (!(fvm_object_set(c, property_depth_receiver, "slot", &value) == expected))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(fvm_object_get(c, property_depth_receiver, "slot", &out) == expected))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (expected == FVM_EXCEPTION)
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
                        code.variable("depthPropertyTarget");
                        code.text("slot");
                        if (setter)
                        {
                            code.number(81);
                            code.op(0x4f);
                            code.number(1);
                        }
                        else
                        {
                            code.op(0x4e);
                        }
                        code.finish();
                        out = boxing_run(c, code, route - 1, expected);
                    }
                    if (expected == FVM_OK)
                    {
                        if (setter)
                        {
                            if (!(fvm_object_get(c, property_depth_receiver, "savedSuper", &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(out.type == FVM_VALUE_OBJECT && out.object->resource_kind == FVM_RESOURCE_SUPER &&
                              out.object->resource_result.object->properties["depth"].number == 1))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH && !c->gc_frames &&
                          !c->gc_native_depth && !c->calling_function && !c->calling_owner &&
                          !c->calling_setter_owner && !c->exception_pending))
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
