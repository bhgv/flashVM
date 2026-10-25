/* Boolean construction uses own stored prototype and versioned constructor identities. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/property.h"

static FVM_Object *boolean_class, *boolean_replacement;
static unsigned boolean_coercions, boolean_getters;


static FVM_Result boolean_getter(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++boolean_getters;
    *out = class_value(boolean_replacement);
    return fvm_exception_raise(c, out);
}


/* Script accessors retain data separately from the throwing getter. */
static FVM_Result boolean_script_getter(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    return boolean_getter(c, 0, 0, 0, out, 0);
}


static FVM_Result boolean_input(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++boolean_coercions;
    *out = class_value(boolean_replacement);
    return fvm_exception_raise(c, out);
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
                if (!(fvm_get_global(c, "Boolean", &klass) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                boolean_class = klass.object;
                if (!(fvm_object_get(c, boolean_class, "prototype", &stored) == FVM_OK &&
                      fvm_gc_pin(c, stored.object) == FVM_OK && fvm_object_create(c, &boolean_replacement) == FVM_OK &&
                      fvm_gc_pin(c, boolean_replacement) == FVM_OK))
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
                if (!(fvm_object_set(c, constructors, "Boolean", &klass) == FVM_OK))
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
                    if (!(fvm_object_set_property_flags(c, boolean_class, "prototype", 0, FVM_PROPERTY_VALID_FLAGS) ==
                          FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_delete(c, boolean_class, "prototype", &removed) == FVM_OK && removed))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (kind == 1)
                    {
                        if (!(fvm_object_define_accessor_rooted(
                                  c, boolean_class, "prototype", boolean_getter, 0, 0, boolean_replacement) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 2)
                    {
                        fvm_value_number(&out, 17);
                        if (!(fvm_object_set(c, boolean_class, "prototype", &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 4)
                    {
                        if (!(fvm_object_define_method_rooted(
                                  c, boolean_class, "prototype", boolean_getter, 0, boolean_replacement) == FVM_OK &&
                              fvm_object_get(c, boolean_class, "prototype", &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        expected = out.object;
                    }
                    else if (kind == 5)
                    {
                        if (!(fvm_object_set(c, boolean_class, "PrOtOtYpE", &stored) == FVM_OK))
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
                            if (!(fvm_object_set(c, boolean_class, "prototype", &stored) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            fvm_value_number(&out, 17);
                            if (!(fvm_object_set(c, boolean_class, "prototype", &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    const unsigned char end[] = {0};
                    FVM_Object *getter = 0;
                    if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &getter) == FVM_OK &&
                          fvm_function_set_body(c, getter, boolean_script_getter, 0, boolean_replacement) == FVM_OK))
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
                        if (!(fvm_object_call(c, boolean_class, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
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
                              fvm_object_set_prototype(c, boolean_class, carrier) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (kind == 9)
                    {
                        out = class_value(getter);
                        if (!(fvm_object_set(c, boolean_class, "__resolve", &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                FVM_Object *argument = 0;
                if (!(fvm_object_create(c, &argument) == FVM_OK &&
                      fvm_object_define_method_rooted(c, argument, "valueOf", boolean_input, 0, 0) == FVM_OK &&
                      fvm_object_define_method_rooted(c, argument, "toString", boolean_input, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                input = class_value(argument);
                if (!(fvm_set_global(c, "booleanInput", &input) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                boolean_coercions = boolean_getters = 0;
                FVM_Object *instance = 0;
                if (!route)
                {
                    if (!(fvm_function_construct(c, boolean_class, 1, &input, &instance) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (route == 1)
                {
                    if (!(fvm_construct(c, "Boolean", 1, &input, &instance) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    class_mode = route >= 5 ? route - 5 : route - 2;
                    Program code;
                    code.variable("booleanInput");
                    code.number(1);
                    if (route >= 5)
                    {
                        code.variable("constructors");
                        code.text("Boolean");
                        code.op(0x53);
                    }
                    else
                    {
                        code.text("Boolean");
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
                if (!(instance && instance->prototype == expected && instance->resource_kind == FVM_RESOURCE_BOOLEAN &&
                      instance->resource_result_valid && instance->resource_result.type == FVM_VALUE_BOOLEAN &&
                      instance->resource_result.boolean && !boolean_coercions && !boolean_getters))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_pin(c, instance) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(c, instance, "__constructor__", &out) == FVM_OK && out.object == boolean_class))
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
                if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->exception_pending))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
