/* Trusted collecting native calls retain a selected capture after registration replacement. */
#define main call_case_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "object_call_case.cpp"

#undef main
static FVM_Object *capture_owner, *capture_object, *capture_argument;
static unsigned capture_source, capture_status, capture_calls;
static FVM_Value capture_input[2];


static FVM_Result capture_replacement(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_number(out, 29);
    return FVM_OK;
}


static bool capture_alive(FVM_Context *c, FVM_Object *object)
{
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        if (c->objects[i] == object)
        {
            return true;
        }
    }
    return false;
}


static FVM_Result capture_callback(
    FVM_Context *c, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out, void *userdata)
{
    ++capture_calls;
    if (!(!c->gc_native_depth && userdata == capture_object && n == 2 && receiver == call_receiver))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (capture_source == 0)
    {
        int removed = 0;
        if (!(fvm_object_delete(c, capture_owner, "run", &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (capture_source == 1)
    {
        if (!(fvm_object_define_method_rooted(c, capture_owner, "run", capture_replacement, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(fvm_register_function_rooted(c, "run", capture_replacement, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_undefined(capture_input);
    fvm_value_undefined(capture_input + 1);
    if (!(fvm_gc_collect(c, 0) == FVM_OK && capture_alive(c, capture_object) && capture_alive(c, capture_argument)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string_size(args) == 3 && !std::memcmp(args[0].string, "a\0b", 3) &&
          args[1].object == capture_argument))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value edge;
    if (!(fvm_object_get(c, capture_object, "self", &edge) == FVM_OK && edge.object == capture_object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    *out = call_object(capture_object);
    return capture_status == 1 ? fvm_exception_raise(c, out) : capture_status == 2 ? FVM_ERROR : FVM_OK;
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (capture_source = 0; capture_source < 3; ++capture_source)
        {
            for (capture_status = 0; capture_status < 3; ++capture_status)
            {
                if (version && version < 4)
                {
                    continue;
                }
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
                if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_construct(c, "Object", 0, 0, &call_receiver) == FVM_OK &&
                      fvm_gc_pin(c, call_receiver) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                capture_owner = call_receiver;
                if (capture_source == 1)
                {
                    if (!(fvm_construct(c, "Object", 0, 0, &capture_owner) == FVM_OK &&
                          fvm_object_set_prototype(c, call_receiver, capture_owner) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_object_create(c, &capture_object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value value = call_object(capture_object);
                if (!(fvm_object_set(c, capture_object, "self", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (capture_source == 2)
                {
                    if (!(fvm_register_function_rooted(c, "run", capture_callback, capture_object, capture_object) ==
                          FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    c->functions.find(fvm_detail::string("run"))->second.collector_entry = true;
                }
                else
                {
                    if (!(fvm_object_define_method_rooted(
                              c, capture_owner, "run", capture_callback, capture_object, capture_object) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    capture_owner->methods.find(fvm_detail::string("run"))->second.collector_entry = true;
                }
                /* collector_entry is internal trusted-bridge state, not a public native GC opt-out. */
                if (!(fvm_object_create(c, &capture_argument) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                value = call_object(capture_argument);
                if (!(fvm_object_set(c, capture_argument, "self", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_string_bytes(&value, "a\0b", 3);
                if (!(fvm_value_to_string(c, &value, capture_input) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                capture_input[1] = call_object(capture_argument);
                capture_calls = 0;
                FVM_Value out;
                FVM_Result status = fvm_object_call(c, call_receiver, "run", 2, capture_input, &out);
                if (!(capture_calls == 1 && status == (capture_status == 1   ? FVM_EXCEPTION
                                                       : capture_status == 2 ? FVM_ERROR
                                                                             : FVM_OK)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!capture_status)
                {
                    if (!(out.type == FVM_VALUE_OBJECT && out.object == capture_object &&
                          fvm_gc_pin(c, out.object) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK &&
                          capture_alive(c, capture_object)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_gc_unpin(c, out.object) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (capture_status == 1)
                {
                    if (!(fvm_exception_get(c, &out) == FVM_OK && out.object == capture_object &&
                          fvm_gc_collect(c, 0) == FVM_OK && capture_alive(c, capture_object)))
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
                fvm_value_undefined(&out);
                if (!(fvm_gc_collect(c, 0) == FVM_OK && !capture_alive(c, capture_object) &&
                      !capture_alive(c, capture_argument)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_owner))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
