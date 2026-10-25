/* Loaded apply uses named properties on native objects, preserving receiver and snapshots. */
#define FVM_APPLY_LOADED_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_apply_loaded.cpp"

static FVM_Object *property_source;
static unsigned property_getter_calls, property_target_calls, property_fault;


static FVM_Result property_getter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++property_getter_calls;
    if (!(frame->receiver == property_source))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value number;
    fvm_value_number(&number, 999);
    if (!(fvm_object_set(c, property_source, "0", &number) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (property_fault == 1)
    {
        return FVM_ERROR;
    }
    if (property_fault == 2)
    {
        FVM_Value error;
        fvm_value_string_bytes(&error, "index\0failed", 12);
        return fvm_exception_raise(c, &error);
    }
    return fvm_value_string_bytes(out, "getter\0result", 13);
}


static FVM_Result property_target(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    ++property_target_calls;
    if (!(property_getter_calls == 1 && n == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a[0].type == FVM_VALUE_STRING && fvm_value_string_size(a) == 3 && !std::memcmp(a[0].string, "a\0b", 3)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a[1].type == FVM_VALUE_STRING && fvm_value_string_size(a + 1) == 13 &&
          !std::memcmp(a[1].string, "getter\0result", 13)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a[2].type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 3);
    return FVM_OK;
}
#ifndef FVM_APPLY_PROPERTIES_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *classes[] = {"Number", "Boolean", "String", "Date"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned kind = 0; kind < 4; ++kind)
        {
            FVM_Context *c = 0;
            FVM_Object *fn = 0, *proto = 0;
            FVM_Value input, value, args[2], out;
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
            if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&input, "abc");
            if (!(fvm_construct(c, classes[kind], kind == 2 ? 1 : 0, &input, &property_source) == FVM_OK &&
                  fvm_gc_pin(c, property_source) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, property_source, "__proto__", &value) == FVM_OK && value.type == FVM_VALUE_OBJECT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_set_prototype(c, proto, value.object) == FVM_OK &&
                  fvm_object_set_prototype(c, property_source, proto) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 3);
            if (!(fvm_object_set(c, property_source, "length", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            apply_getter(c, proto, "1", property_getter);
            if (!(fvm_function_create_native(c, property_target, 0, 0, &fn) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(args);
            args[0].type = FVM_VALUE_NULL;
            args[1] = apply_object(property_source);
            for (unsigned phase = 0; phase < 4; ++phase)
            {
                property_fault = phase == 3 ? 0 : phase;
                property_getter_calls = property_target_calls = 0;
                fvm_value_string_bytes(&value, "a\0b", 3);
                if (!(fvm_object_set(c, property_source, "0", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Result status = fvm_object_call(c, fn, "apply", 2, args, &out);
                if (!(property_getter_calls == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!property_fault)
                {
                    if (!(status == FVM_OK && out.number == 3 && property_target_calls == 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(!property_target_calls && status == (property_fault == 1 ? FVM_ERROR : FVM_EXCEPTION)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (property_fault == 2)
                    {
                        if (!(fvm_exception_get(c, &out) == FVM_OK && fvm_value_string_size(&out) == 12 &&
                              !std::memcmp(out.string, "index\0failed", 12)))
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
                if (!(fvm_object_get(c, property_source, "0", &value) == FVM_OK && value.number == 999))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
#endif
