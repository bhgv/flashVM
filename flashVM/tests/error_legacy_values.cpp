/* Loaded Error.toString returns message unchanged; Error() initializes its caller. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/serialization.h"

static unsigned error_value_traps;


static FVM_Result error_value_trap(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    ++error_value_traps;
    return FVM_ERROR;
}


static void equal_value(const FVM_Value &a, const FVM_Value &b)
{
    if (!(a.type == b.type))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (a.type == FVM_VALUE_NUMBER)
    {
        if (!(a.number == b.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (a.type == FVM_VALUE_BOOLEAN)
    {
        if (!(a.boolean == b.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (a.type == FVM_VALUE_OBJECT)
    {
        if (!(a.object == b.object))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (a.type == FVM_VALUE_STRING)
    {
        if (!(fvm_value_string_size(&a) == fvm_value_string_size(&b) &&
              !std::memcmp(a.string, b.string, fvm_value_string_size(&a))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
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
    for (unsigned version = 4; version <= 8; ++version)
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
        FVM_Value klass, input[6], out, value;
        if (!(fvm_get_global(c, "Error", &klass) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(input);
        fvm_value_undefined(input + 1);
        input[1].type = FVM_VALUE_NULL;
        fvm_value_boolean(input + 2, 1);
        fvm_value_number(input + 3, 3.5);
        if (!(fvm_value_string_bytes(input + 4, "A\0B", 3) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *message = 0, *error = 0, *receiver = 0;
        if (!(fvm_object_create(c, &message) == FVM_OK &&
              fvm_object_define_method_rooted(c, message, "toString", error_value_trap, 0, 0) == FVM_OK &&
              fvm_object_define_method_rooted(c, message, "valueOf", error_value_trap, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        input[5] = class_value(message);
        if (!(fvm_set_global(c, "messageObject", input + 5) == FVM_OK &&
              fvm_construct(c, "Error", 0, 0, &error) == FVM_OK && fvm_object_create(c, &receiver) == FVM_OK &&
              fvm_gc_pin(c, receiver) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = class_value(error);
        if (!(fvm_set_global(c, "errorInstance", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *owner = 0;
        if (!(fvm_object_create(c, &owner) == FVM_OK && fvm_object_set(c, owner, "Error", &klass) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = class_value(owner);
        if (!(fvm_set_global(c, "holders", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        error_value_traps = 0;
        for (unsigned kind = 0; kind < 6; ++kind)
        {
            if (!(fvm_object_set(c, error, "message", input + kind) == FVM_OK &&
                  fvm_object_call(c, error, "toString", 0, 0, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            equal_value(out, input[kind]);
            if (!(fvm_set_global(c, "messageInput", input + kind) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (fvm_jit_available())
            {
                for (class_mode = 0; class_mode < 3; ++class_mode)
                {
                    Program code;
                    code.number(0);
                    code.variable("errorInstance");
                    code.text("toString");
                    code.op(0x52);
                    code.finish();
                    equal_value(class_run(c, code), input[kind]);
                }
            }
            for (unsigned route = 0; route < 8; ++route)
            {
                fvm_value_number(&value, 81);
                if (!(fvm_object_set(c, receiver, "message", &value) == FVM_OK &&
                      fvm_object_set(c, owner, "message", &value) == FVM_OK &&
                      fvm_set_global(c, "message", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *target = route == 1 ? receiver : route >= 5 ? owner : c->global;
                if (route == 0)
                {
                    if (!(fvm_call(c, "Error", 1, input + kind, &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (route == 1)
                {
                    if (!(fvm_function_invoke(c, klass.object, receiver, 1, input + kind, &out) == FVM_OK))
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
                    code.variable("messageInput");
                    code.number(1);
                    if (route >= 5)
                    {
                        code.variable("holders");
                        code.text("Error");
                        code.op(0x52);
                    }
                    else
                    {
                        code.text("Error");
                        code.op(0x3d);
                    }
                    code.finish();
                    out = class_run(c, code);
                }
                if (!(out.type == FVM_VALUE_UNDEFINED && fvm_object_get(c, target, "message", &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                equal_value(out, kind ? input[kind] : value);
            }
            if (!(!error_value_traps && fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        /* Omitted input leaves the existing receiver property untouched. */
        if (!(fvm_object_get(c, receiver, "message", &value) == FVM_OK &&
              fvm_function_invoke(c, klass.object, receiver, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED &&
              fvm_object_get(c, receiver, "message", &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        equal_value(out, value);
        /* The callable initializer keeps its separate graph identity in another loaded context. */
        unsigned char *data = 0;
        size_t size = 0;
        FVM_Context *other = 0;
        if (!(fvm_value_serialize(c, &klass, &data, &size) == FVM_OK && fvm_context_create(runtime, &other) == FVM_OK &&
              fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK &&
              fvm_value_deserialize(other, data, size, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(data);
        FVM_Object *copy = out.object;
        if (!(fvm_gc_pin(other, copy) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 42);
        if (!(fvm_function_invoke(other, copy, 0, 1, &value, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED &&
              fvm_get_global(other, "message", &out) == FVM_OK && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_construct(other, copy, 1, &value, &error) == FVM_OK &&
              fvm_object_call(other, error, "toString", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
              out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->gc_frames && !c->gc_native_depth && !other->gc_frames && !other->gc_native_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
