/* Named constructors retain and snapshot caller arguments before global lookup and callbacks. */
#define main call_case_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "object_call_case.cpp"

#undef main
static FVM_Value global_args[2];
static FVM_Object *global_argument, *global_function;
static unsigned global_mutate, global_fault, global_calls, global_route;


static FVM_Result
global_arguments_result(FVM_Context *c, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out)
{
    ++global_calls;
    if (!(receiver && (global_route == 0 ? receiver == c->global : receiver != c->global)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (global_mutate)
    {
        fvm_value_undefined(global_args);
        fvm_value_undefined(global_args + 1);
    }
    if (!(n == 2 && args[0].type == FVM_VALUE_STRING && fvm_value_string_size(args) == 3 &&
          !std::memcmp(args[0].string, "a\0b", 3)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(args[1].type == FVM_VALUE_OBJECT && args[1].object == global_argument))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!c->gc_native_depth)
    {
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Value marker;
    if (!(fvm_object_get(c, global_argument, "marker", &marker) == FVM_OK && marker.number == 55))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    *out = args[1];
    return global_fault == 1 ? FVM_ERROR : global_fault == 2 ? fvm_exception_raise(c, out) : FVM_OK;
}


static FVM_Result
global_arguments_native(FVM_Context *c, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out, void *)
{
    return global_arguments_result(c, receiver, n, args, out);
}


static FVM_Result global_arguments_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    return global_arguments_result(c, frame->receiver, frame->argc, frame->argv, out);
}


static FVM_Result global_arguments_get(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    if (global_mutate)
    {
        fvm_value_undefined(global_args);
        fvm_value_undefined(global_args + 1);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool alive = false;
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        if (c->objects[i] == global_argument)
        {
            alive = true;
        }
    }
    if (!(alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    *out = call_object(global_function);
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (global_route = 0; global_route < 4; ++global_route)
        {
            for (global_mutate = 0; global_mutate < 2; ++global_mutate)
            {
                for (global_fault = 0; global_fault < 3; ++global_fault)
                {
                    if ((version && version < 4) || (!version && global_route == 3))
                    {
                        continue;
                    }
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(runtime, &c) == FVM_OK))
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
                    const unsigned char end[] = {0};
                    FVM_Value value, out, descriptor[3];
                    const char name[] = "run\0X";
                    if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &global_function) == FVM_OK &&
                          fvm_function_set_body(c, global_function, global_arguments_body, 0, 0) == FVM_OK &&
                          fvm_gc_pin(c, global_function) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = call_object(global_function);
                    if (!global_route)
                    {
                        if (!(fvm_register_constructor_rooted_bytes(
                                  c, name, sizeof(name) - 1, global_arguments_native, 0, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (global_route == 1)
                    {
                        if (!(fvm_set_global_bytes(c, name, sizeof(name) - 1, &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        FVM_Object *getter = 0;
                        if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &getter) == FVM_OK &&
                              fvm_function_set_body(c, getter, global_arguments_get, 0, global_function) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (global_route == 3)
                        {
                            value = call_object(getter);
                            if (!(fvm_set_global(c, "__resolve", &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            fvm_value_string_bytes(descriptor, name, sizeof(name) - 1);
                            descriptor[1] = call_object(getter);
                            fvm_value_undefined(descriptor + 2);
                            descriptor[2].type = FVM_VALUE_NULL;
                            FVM_Object *helper = 0;
                            if (!(fvm_construct(c, "Object", 0, 0, &helper) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Value add;
                            if (!(fvm_object_get(c, helper, "addProperty", &add) == FVM_OK &&
                                  fvm_function_invoke(c, add.object, c->global, 3, descriptor, &out) == FVM_OK &&
                                  out.boolean))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    if (!(fvm_object_create(c, &global_argument) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = call_object(global_argument);
                    if (!(fvm_object_set(c, global_argument, "self", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_number(&value, 55);
                    if (!(fvm_object_set(c, global_argument, "marker", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_string_bytes(&value, "a\0b", 3);
                    if (!(fvm_value_to_string(c, &value, global_args) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    global_args[1] = call_object(global_argument);
                    global_calls = 0;
                    FVM_Object *output = 0;
                    FVM_Result expected = global_fault == 1 ? FVM_ERROR : global_fault == 2 ? FVM_EXCEPTION : FVM_OK;
                    if (!(fvm_construct_bytes(c, name, sizeof(name) - 1, 2, global_args, &output) == expected &&
                          global_calls == 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (expected == FVM_OK)
                    {
                        if (!(version && global_route ? output && output != global_argument
                                                      : output == global_argument))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (expected == FVM_EXCEPTION)
                    {
                        if (!(fvm_exception_get(c, &value) == FVM_OK && value.object == global_argument &&
                              fvm_exception_clear(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    fvm_value_undefined(global_args);
                    fvm_value_undefined(global_args + 1);
                    if (!(!c->gc_frames && !c->gc_native_depth && !c->resolve_depth && fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    for (size_t i = 0; i < c->objects.size(); ++i)
                    {
                        if (!(c->objects[i] != global_argument))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
