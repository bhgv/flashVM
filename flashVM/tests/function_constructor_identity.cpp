/* Script construction defines own hidden constructor identities before running the body. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/serialization.h"
#include "flashvm/table.h"

static unsigned identity_version, identity_calls, identity_traps;


static FVM_Result identity_trap(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++identity_traps;
    fvm_value_number(out, 91);
    return FVM_OK;
}


static void identity_check(FVM_Context *c, FVM_Object *instance, FVM_Object *function)
{
    const char *keys[] = {"__constructor__", "constructor"};
    for (unsigned i = 0; i < 2; ++i)
    {
        bool own = identity_version && (!i || identity_version < 7);
        int found = 9;
        if (!(fvm_object_has_own(c, instance, keys[i], &found) == FVM_OK && !!found == own))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (own)
        {
            FVM_Value value;
            unsigned flags = 0;
            if (!(fvm_object_get(c, instance, keys[i], &value) == FVM_OK && value.type == FVM_VALUE_OBJECT &&
                  value.object == function))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_property_flags(c, instance, keys[i], &flags) == FVM_OK && flags == FVM_PROPERTY_DONT_ENUM))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
}


static FVM_Result identity_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++identity_calls;
    identity_check(c, frame->receiver, frame->function);
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
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
    for (identity_version = 0; identity_version <= 8; ++identity_version)
    {
        for (unsigned poison = 0; poison < 2; ++poison)
        {
            if (identity_version && identity_version < 4)
            {
                continue;
            }
            FVM_Context *c = 0;
            if (!(fvm_context_create(runtime, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (identity_version)
            {
                unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                swf[3] = (unsigned char)identity_version;
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
            const unsigned char end[] = {0};
            FVM_Object *function = 0, *instance = 0;
            if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &function) == FVM_OK &&
                  fvm_function_set_body(c, function, identity_body, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value value = class_value(function), proto;
            if (!(fvm_set_global(c, "IdentityCtor", &value) == FVM_OK &&
                  fvm_object_get(c, function, "prototype", &proto) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (poison)
            {
                int removed = 0;
                if (!(fvm_object_delete(c, proto.object, "constructor", &removed) == FVM_OK && removed))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_define_accessor_rooted(
                          c, proto.object, "constructor", identity_trap, identity_trap, 0, 0) == FVM_OK &&
                      fvm_object_define_accessor_rooted(
                          c, proto.object, "__constructor__", identity_trap, identity_trap, 0, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            identity_calls = identity_traps = 0;
            if (!(fvm_function_construct(c, function, 0, 0, &instance) == FVM_OK && fvm_gc_pin(c, instance) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            identity_check(c, instance, function);
            if (!(identity_calls == 1 && !identity_traps))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *names = 0;
            FVM_Value length;
            if (!(fvm_object_enumerate(c, instance, 0, &names) == FVM_OK &&
                  fvm_object_get(c, names, "length", &length) == FVM_OK && length.number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!poison && identity_version)
            {
                unsigned char *data = 0;
                size_t size = 0;
                value = class_value(instance);
                FVM_Context *other = 0;
                if (!(fvm_value_serialize(c, &value, &data, &size) == FVM_OK &&
                      fvm_context_create(runtime, &other) == FVM_OK &&
                      fvm_value_deserialize(other, data, size, &value) == FVM_OK &&
                      fvm_gc_pin(other, value.object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_serialized_free(data);
                FVM_Value constructor;
                if (!(fvm_object_get(other, value.object, "__constructor__", &constructor) == FVM_OK &&
                      constructor.type == FVM_VALUE_OBJECT && constructor.object->owner == other &&
                      constructor.object != function))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                identity_check(other, value.object, constructor.object);
                if (!(fvm_gc_collect(other, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                identity_check(other, value.object, constructor.object);
                fvm_context_destroy(other);
            }
            if (identity_version)
            {
                fvm_value_number(&value, 17);
                int removed = 0;
                if (!(fvm_object_set(c, instance, "__constructor__", &value) == FVM_OK &&
                      fvm_object_get(c, instance, "__constructor__", &value) == FVM_OK && value.number == 17 &&
                      fvm_object_delete(c, instance, "__constructor__", &removed) == FVM_OK && removed))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (fvm_jit_available())
            {
                for (class_mode = 0; class_mode < 3; ++class_mode)
                {
                    Program code;
                    code.number(0);
                    code.text("IdentityCtor");
                    code.op(0x40);
                    code.finish();
                    value = class_run(c, code);
                    identity_check(c, value.object, function);
                }
            }
            if (!(!identity_traps && !c->gc_frames && !c->gc_native_depth && !c->script_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
