/* Special parent conversions retain the original receiver/owner through collection and errors. */
#define FVM_SUPER_ACCESSOR_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "object_super_accessors.cpp"
#include "flashvm/execution.h"
#include "flashvm/property.h"
#include "flashvm/exception.h"
#include "../src/avm1/coercion.hpp"
#include <cstring>

static FVM_Object *conversion_receiver, *conversion_owner;
static unsigned conversion_kind, conversion_fault, conversion_calls;


static FVM_Result parent_conversion(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++conversion_calls;
    if (!(frame->receiver == conversion_receiver && c->calling_owner == conversion_owner))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (conversion_fault == 3)
    {
        int removed = 0;
        if (!(fvm_object_delete(c, conversion_owner, conversion_kind == 1 ? "toString" : "valueOf", &removed) ==
                  FVM_OK &&
              removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_prototype(c, conversion_receiver, 0) == FVM_OK))
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
    FVM_Value marker;
    if (!(fvm_object_get(c, frame->receiver, "marker", &marker) == FVM_OK && marker.number == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (conversion_fault == 1)
    {
        fvm_value_number(out, 81);
        return fvm_exception_raise(c, out);
    }
    if (conversion_fault == 2)
    {
        return FVM_ERROR;
    }
    if (conversion_kind == 1)
    {
        return fvm_value_string_bytes(out, "r\0s", 3);
    }
    *out = marker;
    return FVM_OK;
}
#ifdef FVM_SUPER_CONVERSION_JIT
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"

#endif


static FVM_Result convert_parent(FVM_Context *c, FVM_Object *facade, FVM_Value *out)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = facade;
#ifdef FVM_SUPER_CONVERSION_JIT
    if (!(fvm_set_global(c, "conversionTarget", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Program code;
    code.variable("conversionTarget");
    if (conversion_kind == 2)
    {
        code.number(0);
        code.op(0x47);
    }
    else
    {
        code.op(conversion_kind == 1 ? 0x4b : 0x4a);
    }
    code.finish();
    FVM_Result status = conversion_fault == 1 ? FVM_EXCEPTION : conversion_fault == 2 ? FVM_ERROR : FVM_OK;
    *out = class_run(c, code, status);
    return status;
#else
    if (conversion_kind == 2)
    {
        return fvm_avm1_primitive(c, &value, true, out);
    }
    *out = value;
    return conversion_kind == 1 ? fvm_exec_to_string(c, out, out) : fvm_exec_to_number(c, 0, out, out);
#endif
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
#ifdef FVM_SUPER_CONVERSION_JIT
    if (!fvm_jit_available())
    {
        fvm_runtime_destroy(rt);
        return 0;
    }
    for (class_mode = 0; class_mode < 3; ++class_mode)
#endif
        for (unsigned version = 5; version <= 8; ++version)
        {
            for (unsigned spelling = 0; spelling < 2; ++spelling)
            {
                for (conversion_kind = 0; conversion_kind < 3; ++conversion_kind)
                {
                    for (conversion_fault = 0; conversion_fault < 4; ++conversion_fault)
                    {
                        if (spelling && version >= 7)
                        {
                            continue;
                        }
                        FVM_Context *c = 0;
                        if (!(fvm_context_create(rt, &c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                        swf[3] = (unsigned char)version;
                        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Object *target = 0, *facade = 0, *method = 0;
                        super_accessor_setup(c, &conversion_receiver, &target, &facade);
                        FVM_Value base, value;
                        if (!(fvm_get_global(c, "Base", &base) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        conversion_owner = prototype(c, base);
                        if (!conversion_fault)
                        {
                            for (unsigned noncallable = 0; noncallable < 2; ++noncallable)
                            {
                                fvm_value_number(&value, 17);
                                if (noncallable)
                                {
                                    value.type = FVM_VALUE_OBJECT;
                                    value.object = target;
                                }
                                if (!(fvm_object_set(
                                          c, conversion_owner, conversion_kind == 1 ? "toString" : "valueOf", &value) ==
                                      FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                FVM_Value input;
                                fvm_value_undefined(&input);
                                input.type = FVM_VALUE_OBJECT;
                                input.object = facade;
                                if (conversion_kind == 1)
                                {
                                    if (!(fvm_exec_to_string(c, &input, &value) == FVM_OK &&
                                          !std::strcmp(value.string, "[type Object]")))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                else
                                {
                                    if (!(fvm_exec_to_number(c, 0, &input, &value) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                    if (!(version < 7 ? value.number == 0 : value.number != value.number))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                int removed = 0;
                                if (!(fvm_object_delete(c,
                                                        conversion_owner,
                                                        conversion_kind == 1 ? "toString" : "valueOf",
                                                        &removed) == FVM_OK &&
                                      removed))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                        const unsigned char end[] = {0};
                        if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &method) == FVM_OK &&
                              fvm_function_set_body(c, method, parent_conversion, 0, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_value_undefined(&value);
                        value.type = FVM_VALUE_OBJECT;
                        value.object = method;
                        if (!(fvm_object_set(c,
                                             conversion_owner,
                                             conversion_kind == 1 ? (spelling ? "TOSTRING" : "toString")
                                                                  : (spelling ? "VALUEOF" : "valueOf"),
                                             &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        conversion_calls = 0;
                        FVM_Result status = convert_parent(c, facade, &value);
                        if (!(conversion_calls == 1 && status == (conversion_fault == 1   ? FVM_EXCEPTION
                                                                  : conversion_fault == 2 ? FVM_ERROR
                                                                                          : FVM_OK)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (status == FVM_OK)
                        {
                            if (conversion_kind == 1)
                            {
                                if (!(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 &&
                                      !std::memcmp(value.string, "r\0s", 3)))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            else
                            {
                                if (!(value.type == FVM_VALUE_NUMBER && value.number == 9))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
#ifndef FVM_SUPER_CONVERSION_JIT
                        if (status == FVM_EXCEPTION)
                        {
                            if (!(fvm_exception_get(c, &value) == FVM_OK && value.number == 81 &&
                                  fvm_exception_clear(c) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
#endif
                        if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function &&
                              !c->primitive_depth && !c->string_depth))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_gc_collect(c, 0) == FVM_OK))
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
