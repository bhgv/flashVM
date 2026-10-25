/* Loaded writes skip intermediate data but respect own membership and virtual visibility. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"
#include "flashvm/exception.h"

static unsigned deep_calls;
static double deep_length;
static bool deep_array;


static FVM_Result deep_get(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    fvm_value_number(out, 17);
    return FVM_OK;
}


static FVM_Result deep_set(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++deep_calls;
    if (!(frame->argc == 1 && frame->argv[0].number == 81))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (deep_array)
    {
        FVM_Value length;
        if (!(fvm_object_get(c, frame->receiver, "length", &length) == FVM_OK && length.number == deep_length))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Value value;
    fvm_value_number(&value, 1);
    if (!(fvm_object_set(c, frame->receiver, "called", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result deep_method(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_number(out, 99);
    return FVM_OK;
}


static void deep_property(FVM_Context *c, FVM_Object *owner, const char *key, FVM_Object *get, FVM_Object *set)
{
    FVM_Value args[3], out;
    fvm_value_string(args, key);
    args[1] = class_value(get);
    args[2] = class_value(set);
    if (!(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    const char *keys[] = {"slot", "5", "+5"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned array = 0; array < 2; ++array)
        {
            for (unsigned key = 0; key < 3; ++key)
            {
                for (unsigned own = 0; own < 5; ++own)
                {
                    for (unsigned hidden = 0; hidden < 2; ++hidden)
                    {
                        for (unsigned middle_method = 0; middle_method < 2; ++middle_method)
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
                                FVM_Object *receiver = 0, *middle = 0, *far = 0, *get = 0, *set = 0;
                                if (!(fvm_construct(c, array ? "Array" : "Object", 0, 0, &receiver) == FVM_OK &&
                                      fvm_gc_pin(c, receiver) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (!(fvm_construct(c, "Object", 0, 0, &middle) == FVM_OK &&
                                      fvm_gc_pin(c, middle) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (!(fvm_construct(c, "Object", 0, 0, &far) == FVM_OK && fvm_gc_pin(c, far) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                const unsigned char end[] = {0};
                                if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK &&
                                      fvm_function_set_body(c, get, deep_get, 0, 0) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &set) == FVM_OK &&
                                      fvm_function_set_body(c, set, deep_set, 0, 0) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                deep_property(c, far, keys[key], get, set);
                                if (hidden)
                                {
                                    version_flags(c, far, keys[key], version_masks[version - 4], 65535);
                                }
                                FVM_Value value, out, target = class_value(receiver);
                                fvm_value_number(&value, 9);
                                if (middle_method)
                                {
                                    if (!(fvm_object_define_method_rooted(c, middle, keys[key], deep_method, 0, 0) ==
                                          FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                else
                                {
                                    if (!(fvm_object_set(c, middle, keys[key], &value) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                if (!(fvm_object_set_property_flags(c, middle, keys[key], 4, 0) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (own == 1 || own == 4)
                                {
                                    if (!(fvm_object_set(c, receiver, keys[key], &value) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                if (own == 2)
                                {
                                    if (!(fvm_object_define_method_rooted(c, receiver, keys[key], deep_method, 0, 0) ==
                                          FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                if (own == 2)
                                {
                                    version_flags(c, receiver, keys[key], version_masks[version - 4], 65535);
                                }
                                if (own == 3)
                                {
                                    deep_property(c, receiver, keys[key], get, set);
                                }
                                if (own == 4)
                                {
                                    version_flags(c, receiver, keys[key], version_masks[version - 4] | 4, 65535);
                                }
                                /* Retain Array prototype length and Object methods behind the descriptor owner. */
                                if (!(fvm_object_set_prototype(c, far, receiver->prototype) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (!(fvm_object_set_prototype(c, middle, far) == FVM_OK &&
                                      fvm_object_set_prototype(c, receiver, middle) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                deep_calls = 0;
                                deep_array = array != 0;
                                deep_length = array && key && (own || hidden) ? 6 : 0;
                                fvm_value_number(&value, 81);
                                if (!(fvm_set_global(c, "deepReceiver", &target) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (!route)
                                {
                                    if (!(fvm_object_set(c, receiver, keys[key], &value) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                else
                                {
                                    Program code;
                                    code.variable("deepReceiver");
                                    code.text(keys[key]);
                                    code.number(81);
                                    code.op(0x4f);
                                    code.number(1);
                                    code.finish();
                                    out = boxing_run(c, code, route - 1, FVM_OK);
                                }
                                if (!(deep_calls == (own == 3 || (!own && !hidden) ? 1U : 0U)))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (array)
                                {
                                    if (!(fvm_object_get(c, receiver, "length", &out) == FVM_OK &&
                                          out.number == deep_length))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                int present = 0;
                                if (!(fvm_object_has_own(c, receiver, keys[key], &present) == FVM_OK &&
                                      !!present == !!(own || hidden)))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (own == 4)
                                {
                                    version_flags(c, receiver, keys[key], 0, 65528);
                                    if (!(fvm_object_get(c, receiver, keys[key], &out) == FVM_OK && out.number == 9))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner &&
                                      c->watch_running.empty()))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                fvm_context_destroy(c);
                            }
                        }
                    }
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
