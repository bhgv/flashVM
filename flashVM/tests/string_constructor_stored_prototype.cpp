/* String construction snapshots stored prototype and identities before argument coercion. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/property.h"

static FVM_Object *string_class, *string_replacement;
static unsigned string_coercions, string_getters;
static bool string_mutate;
static FVM_Result string_failure;


static FVM_Result string_getter(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++string_getters;
    *out = class_value(string_replacement);
    return fvm_exception_raise(c, out);
}


/* Script accessors retain data separately from the throwing getter. */
static FVM_Result string_script_getter(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    return string_getter(c, 0, 0, 0, out, 0);
}


static FVM_Result string_input(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++string_coercions;
    if (string_mutate)
    {
        FVM_Value value = class_value(string_replacement);
        int removed = 0;
        if (!(fvm_object_set_property_flags(c, string_class, "prototype", 0, FVM_PROPERTY_VALID_FLAGS) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_delete(c, string_class, "prototype", &removed) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, string_class, "prototype", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (string_failure == FVM_EXCEPTION)
    {
        *out = class_value(string_replacement);
        return fvm_exception_raise(c, out);
    }
    if (string_failure != FVM_OK)
    {
        return string_failure;
    }
    fvm_value_string(out, "a\0b");
    out->string_size = 3;
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned kind = 0; kind < 10; ++kind)
        {
            for (unsigned mutate = 0; mutate < 2; ++mutate)
            {
                for (unsigned route = 0; route < 8; ++route)
                {
                    if (route >= 2 && !fvm_jit_available())
                    {
                        continue;
                    }
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(runtime, &c) == FVM_OK))
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
                    FVM_Value klass, stored, input, out;
                    if (!(fvm_get_global(c, "String", &klass) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    string_class = klass.object;
                    if (!(fvm_object_get(c, string_class, "prototype", &stored) == FVM_OK &&
                          fvm_gc_pin(c, stored.object) == FVM_OK &&
                          fvm_object_create(c, &string_replacement) == FVM_OK &&
                          fvm_gc_pin(c, string_replacement) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *constructors = 0;
                    if (!(fvm_object_create(c, &constructors) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_set(c, constructors, "String", &klass) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    out = class_value(constructors);
                    if (!(fvm_set_global(c, "constructors", &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *expected = stored.object;
                    if (kind)
                    {
                        int removed = 0;
                        if (!(fvm_object_set_property_flags(
                                  c, string_class, "prototype", 0, FVM_PROPERTY_VALID_FLAGS) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_delete(c, string_class, "prototype", &removed) == FVM_OK && removed))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (kind == 1)
                        {
                            if (!(fvm_object_define_accessor_rooted(
                                      c, string_class, "prototype", string_getter, 0, 0, string_replacement) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind == 2)
                        {
                            fvm_value_number(&out, 17);
                            if (!(fvm_object_set(c, string_class, "prototype", &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind == 4)
                        {
                            if (!(fvm_object_define_method_rooted(
                                      c, string_class, "prototype", string_getter, 0, string_replacement) == FVM_OK &&
                                  fvm_object_get(c, string_class, "prototype", &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            expected = out.object;
                        }
                        else if (kind == 5)
                        {
                            if (!(fvm_object_set(c, string_class, "PrOtOtYpE", &stored) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            expected = version < 7 ? stored.object : 0;
                        }
                        else
                        {
                            expected = 0;
                        }
                    }
                    if (kind >= 6)
                    {
                        if (kind == 6 || kind == 7)
                        {
                            if (kind == 6)
                            {
                                if (!(fvm_object_set(c, string_class, "prototype", &stored) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            else
                            {
                                fvm_value_number(&out, 17);
                                if (!(fvm_object_set(c, string_class, "prototype", &out) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                        const unsigned char end[] = {0};
                        FVM_Object *getter = 0;
                        if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &getter) == FVM_OK &&
                              fvm_function_set_body(c, getter, string_script_getter, 0, string_replacement) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (kind == 6 || kind == 7)
                        {
                            FVM_Value args[3];
                            fvm_value_string(args, "prototype");
                            args[1] = class_value(getter);
                            fvm_value_undefined(args + 2);
                            args[2].type = FVM_VALUE_NULL;
                            if (!(fvm_object_call(c, string_class, "addProperty", 3, args, &out) == FVM_OK &&
                                  out.boolean))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            expected = kind == 6 ? stored.object : 0;
                        }
                        if (kind == 8)
                        {
                            FVM_Object *carrier = 0;
                            if (!(fvm_object_create(c, &carrier) == FVM_OK &&
                                  fvm_object_set(c, carrier, "prototype", &stored) == FVM_OK &&
                                  fvm_object_set_prototype(c, string_class, carrier) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind == 9)
                        {
                            out = class_value(getter);
                            if (!(fvm_object_set(c, string_class, "__resolve", &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    FVM_Object *argument = 0;
                    if (!(fvm_object_create(c, &argument) == FVM_OK &&
                          fvm_object_define_method_rooted(c, argument, "toString", string_input, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    input = class_value(argument);
                    if (!(fvm_set_global(c, "stringInput", &input) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    string_mutate = mutate != 0;
                    string_coercions = string_getters = 0;
                    string_failure = FVM_OK;
                    FVM_Object *instance = 0;
                    if (!route)
                    {
                        if (!(fvm_function_construct(c, string_class, 1, &input, &instance) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (route == 1)
                    {
                        if (!(fvm_construct(c, "String", 1, &input, &instance) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        class_mode = route >= 5 ? route - 5 : route - 2;
                        Program code;
                        code.variable("stringInput");
                        code.number(1);
                        if (route >= 5)
                        {
                            code.variable("constructors");
                            code.text("String");
                            code.op(0x53);
                        }
                        else
                        {
                            code.text("String");
                            code.op(0x40);
                        }
                        code.finish();
                        out = class_run(c, code);
                        if (!(out.type == FVM_VALUE_OBJECT))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        instance = out.object;
                    }
                    if (!(instance && instance->prototype == expected &&
                          instance->resource_kind == FVM_RESOURCE_STRING && instance->resource_result_valid &&
                          instance->resource_result.type == FVM_VALUE_STRING &&
                          fvm_string_size(instance->resource_result) == 3 &&
                          !std::memcmp(instance->resource_result.string, "a\0b", 3) && string_coercions == 1 &&
                          !string_getters))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_get(c, instance, "length", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
                          out.number == 3))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_gc_pin(c, instance) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_get(c, instance, "__constructor__", &out) == FVM_OK && out.object == string_class))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    int own = 0;
                    if (!(fvm_object_has_own(c, instance, "constructor", &own) == FVM_OK && !!own == (version < 7)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    for (unsigned failure = 0; failure < 2; ++failure)
                    {
                        string_failure = failure ? FVM_EXCEPTION : FVM_ERROR;
                        string_coercions = 0;
                        if (route < 2)
                        {
                            instance = (FVM_Object *)1;
                            FVM_Result status = route ? fvm_construct(c, "String", 1, &input, &instance)
                                                      : fvm_function_construct(c, string_class, 1, &input, &instance);
                            if (!(status == string_failure && !instance))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (failure)
                            {
                                if (!(c->exception_pending && c->exception_value.object == string_replacement))
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
                            class_mode = route >= 5 ? route - 5 : route - 2;
                            Program code;
                            code.variable("stringInput");
                            code.number(1);
                            if (route >= 5)
                            {
                                code.variable("constructors");
                                code.text("String");
                                code.op(0x53);
                            }
                            else
                            {
                                code.text("String");
                                code.op(0x40);
                            }
                            code.finish();
                            class_run(c, code, string_failure);
                        }
                        if (!(string_coercions == 1 && !string_getters && !c->string_depth))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth &&
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
    fvm_runtime_destroy(runtime);
    return 0;
}
