/* Loaded Object()/Array() differ from construction and survive callback graph transport. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/property.h"
#include "flashvm/serialization.h"
#include "flashvm/table.h"

static unsigned base_traps;


static FVM_Result base_trap(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    ++base_traps;
    return FVM_ERROR;
}


static void check_result(
    FVM_Context *c, const FVM_Value &out, unsigned type, FVM_Object *builtin, unsigned count, const FVM_Value &input)
{
    if (!(out.type == FVM_VALUE_OBJECT && out.object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *object = out.object;
    if (!(object->prototype == (type ? builtin : 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int own = 0;
    if (!(fvm_object_has_own(c, object, "__constructor__", &own) == FVM_OK && !own))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_has_own(c, object, "constructor", &own) == FVM_OK && !own))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (type)
    {
        FVM_Value value;
        if (!(fvm_object_get(c, object, "length", &value) == FVM_OK && value.number == count))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (count)
        {
            if (!(fvm_table_get_index(c, object, 0, &value) == FVM_OK && value.type == input.type))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
}


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *names[] = {"Object", "Array"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned type = 0; type < 2; ++type)
        {
            for (unsigned kind = 0; kind < 4; ++kind)
            {
                for (unsigned argument = 0; argument < 3; ++argument)
                {
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
                    FVM_Value klass, original, input, out;
                    if (!(fvm_get_global(c, names[type], &klass) == FVM_OK &&
                          fvm_object_get(c, klass.object, "prototype", &original) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *builtin = original.object;
                    if (!(fvm_gc_pin(c, builtin) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (kind)
                    {
                        int removed = 0;
                        if (!(fvm_object_set_property_flags(
                                  c, klass.object, "prototype", 0, FVM_PROPERTY_VALID_FLAGS) == FVM_OK &&
                              fvm_object_delete(c, klass.object, "prototype", &removed) == FVM_OK && removed))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (kind == 1)
                        {
                            FVM_Object *replacement = 0;
                            if (!(fvm_object_create(c, &replacement) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            out = class_value(replacement);
                            if (!(fvm_object_set(c, klass.object, "prototype", &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind == 2)
                        {
                            if (!(fvm_object_define_accessor_rooted(c, klass.object, "prototype", base_trap, 0, 0, 0) ==
                                  FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    fvm_value_undefined(&input);
                    if (argument == 2)
                    {
                        input.type = FVM_VALUE_NULL;
                    }
                    unsigned count = argument ? 1 : 0;
                    if (!(fvm_set_global(c, "callInput", &input) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *owner = 0;
                    if (!(fvm_object_create(c, &owner) == FVM_OK &&
                          fvm_object_set(c, owner, names[type], &klass) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    out = class_value(owner);
                    if (!(fvm_set_global(c, "holders", &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    base_traps = 0;
                    for (unsigned route = 0; route < 8; ++route)
                    {
                        if (route == 0)
                        {
                            if (!(fvm_call(c, names[type], count, count ? &input : 0, &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (route == 1)
                        {
                            if (!(fvm_function_invoke(c, klass.object, owner, count, count ? &input : 0, &out) ==
                                  FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!fvm_jit_available())
                            {
                                continue;
                            }
                            class_mode = route >= 5 ? route - 5 : route - 2;
                            Program code;
                            if (count)
                            {
                                code.variable("callInput");
                            }
                            code.number(count);
                            if (route >= 5)
                            {
                                code.variable("holders");
                                code.text(names[type]);
                                code.op(0x52);
                            }
                            else
                            {
                                code.text(names[type]);
                                code.op(0x3d);
                            }
                            code.finish();
                            out = class_run(c, code);
                        }
                        check_result(c, out, type, builtin, count, input);
                        if (!(fvm_gc_pin(c, out.object) == FVM_OK && !base_traps))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    /* A distinct callable ID must survive into a loaded context without standard installation. */
    for (unsigned type = 0; type < 2; ++type)
    {
        FVM_Context *c = 0, *other = 0;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_context_create(runtime, &c) == FVM_OK && fvm_context_create(runtime, &other) == FVM_OK &&
              fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value klass, out;
        if (!(fvm_get_global(c, names[type], &klass) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char *data = 0;
        size_t size = 0;
        if (!(fvm_value_serialize(c, &klass, &data, &size) == FVM_OK &&
              fvm_value_deserialize(other, data, size, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(data);
        if (!(fvm_gc_pin(other, out.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *function = out.object;
        if (!(fvm_function_invoke(other, function, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int own = 0;
        if (!(fvm_object_has_own(other, out.object, "__constructor__", &own) == FVM_OK && !own))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (type)
        {
            FVM_Value value;
            if (!(fvm_object_call(other, out.object, "push", 0, 0, &value) == FVM_OK && value.number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(!out.object->prototype))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
