/* A collecting setter recreates its property with another spelling before data commit. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include "../src/standard/base_state.hpp"

static FVM_Object *fallback, *replacement;
static unsigned replacement_calls, replacement_mode;


static FVM_Result replacement_set(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++replacement_calls;
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result recreate(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    FVM_Object *o = frame->receiver;
    int removed = 0;
    if (!(fvm_object_delete(c, o, "Slot", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_number(&value, 17);
    if (!(fvm_object_set(c, o, "sLOT", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (replacement_mode != 0)
    {
        FVM_Value args[3];
        fvm_value_string(args, "sLOT");
        args[1] = object(fallback);
        args[2] = object(replacement);
        if (!(fvm_object_call(c, o, "addProperty", 3, args, &value) == FVM_OK && value.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (replacement_mode == 2)
    {
        if (!(fvm_object_set_property_flags(c, o, "sLOT", FVM_PROPERTY_READ_ONLY, 0) == FVM_OK))
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
    fvm_value_undefined(out);
    return FVM_OK;
}
#ifdef FVM_SETTER_CASE_JIT
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"

#endif


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef FVM_SETTER_CASE_JIT
    if (!fvm_jit_available())
    {
        fvm_runtime_destroy(rt);
        return 0;
    }
    for (class_mode = 0; class_mode < 3; ++class_mode)
#endif
        for (unsigned version = 0; version <= 8; ++version)
        {
            if (version && version < 4)
            {
                continue;
            }
#ifdef FVM_SETTER_CASE_JIT
            if (!version)
            {
                continue;
            }
#endif
            for (replacement_mode = 0; replacement_mode < 3; ++replacement_mode)
            {
                FVM_Context *c = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (version)
                {
                    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                    swf[3] = (unsigned char)version;
                    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_standard_install(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *o = 0, *setter = 0;
                if (!(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK && fvm_gc_pin(c, o) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_construct(c, "Object", 0, 0, &fallback) == FVM_OK && fvm_gc_pin(c, fallback) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_function_create_native(c, replacement_set, 0, 0, &replacement) == FVM_OK &&
                      fvm_gc_pin(c, replacement) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const unsigned char end[] = {0};
                if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &setter) == FVM_OK &&
                      fvm_function_set_body(c, setter, recreate, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value args[3], value;
                fvm_value_string(args, "Slot");
                args[1] = object(fallback);
                args[2] = object(setter);
                if (!(fvm_object_call(c, o, "addProperty", 3, args, &value) == FVM_OK && value.boolean))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                replacement_calls = 0;
                fvm_value_string_bytes(&value, "a\0b", 3);
#ifdef FVM_SETTER_CASE_JIT
                FVM_Value global = object(o);
                if (!(fvm_set_global(c, "caseTarget", &global) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_set_global(c, "caseValue", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program write;
                write.variable("caseTarget");
                write.text("Slot");
                write.variable("caseValue");
                write.op(0x4f);
                write.number(99);
                write.finish();
                value = class_run(c, write);
                if (!(value.number == 99 && replacement_calls == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
#else
            if (!(fvm_object_set(c, o, "Slot", &value) == FVM_OK && replacement_calls == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
#endif
                if (!(!c->gc_frames && !c->calling_setter_owner && !c->calling_setter_name))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(c, o, "sLOT", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                bool folded = version && version <= 6;
                if (folded && replacement_mode != 2)
                {
                    if (!(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 &&
                          !std::memcmp(value.string, "a\0b", 3)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(value.type == FVM_VALUE_NUMBER && value.number == 17))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!((o->properties.find(fvm_detail::string("Slot")) == o->properties.end()) == folded))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    fvm_runtime_destroy(rt);
}
