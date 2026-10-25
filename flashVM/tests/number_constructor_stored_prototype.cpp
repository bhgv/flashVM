/* Number construction snapshots stored prototype and identities before argument coercion. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/property.h"

static FVM_Object *number_class, *number_replacement;
static unsigned number_coercions, number_getters;
static bool number_mutate;


static FVM_Result number_getter(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++number_getters;
    *out = class_value(number_replacement);
    return fvm_exception_raise(c, out);
}


static FVM_Result number_input(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++number_coercions;
    if (number_mutate)
    {
        FVM_Value value = class_value(number_replacement);
        int removed = 0;
        if (!(fvm_object_set_property_flags(c, number_class, "prototype", 0, FVM_PROPERTY_VALID_FLAGS) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_delete(c, number_class, "prototype", &removed) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, number_class, "prototype", &value) == FVM_OK))
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
    fvm_value_number(out, 42);
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
        for (unsigned kind = 0; kind < 6; ++kind)
        {
            for (unsigned mutate = 0; mutate < 2; ++mutate)
            {
                for (unsigned route = 0; route < 5; ++route)
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
                    if (!(fvm_get_global(c, "Number", &klass) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    number_class = klass.object;
                    if (!(fvm_object_get(c, number_class, "prototype", &stored) == FVM_OK &&
                          fvm_gc_pin(c, stored.object) == FVM_OK &&
                          fvm_object_create(c, &number_replacement) == FVM_OK &&
                          fvm_gc_pin(c, number_replacement) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *expected = stored.object;
                    if (kind)
                    {
                        int removed = 0;
                        if (!(fvm_object_set_property_flags(
                                  c, number_class, "prototype", 0, FVM_PROPERTY_VALID_FLAGS) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_delete(c, number_class, "prototype", &removed) == FVM_OK && removed))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (kind == 1)
                        {
                            if (!(fvm_object_define_accessor_rooted(
                                      c, number_class, "prototype", number_getter, 0, 0, number_replacement) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind == 2)
                        {
                            fvm_value_number(&out, 17);
                            if (!(fvm_object_set(c, number_class, "prototype", &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind == 4)
                        {
                            if (!(fvm_object_define_method_rooted(
                                      c, number_class, "prototype", number_getter, 0, number_replacement) == FVM_OK &&
                                  fvm_object_get(c, number_class, "prototype", &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            expected = out.object;
                        }
                        else if (kind == 5)
                        {
                            if (!(fvm_object_set(c, number_class, "PrOtOtYpE", &stored) == FVM_OK))
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
                    FVM_Object *argument = 0;
                    if (!(fvm_object_create(c, &argument) == FVM_OK &&
                          fvm_object_define_method_rooted(c, argument, "valueOf", number_input, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    input = class_value(argument);
                    if (!(fvm_set_global(c, "numberInput", &input) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    number_mutate = mutate != 0;
                    number_coercions = number_getters = 0;
                    FVM_Object *instance = 0;
                    if (!route)
                    {
                        if (!(fvm_function_construct(c, number_class, 1, &input, &instance) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (route == 1)
                    {
                        if (!(fvm_construct(c, "Number", 1, &input, &instance) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        class_mode = route - 2;
                        Program code;
                        code.variable("numberInput");
                        code.number(1);
                        code.text("Number");
                        code.op(0x40);
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
                          instance->resource_kind == FVM_RESOURCE_NUMBER && instance->resource_result_valid &&
                          instance->resource_result.number == 42 && number_coercions == 1 && !number_getters))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_gc_pin(c, instance) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_get(c, instance, "__constructor__", &out) == FVM_OK && out.object == number_class))
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
