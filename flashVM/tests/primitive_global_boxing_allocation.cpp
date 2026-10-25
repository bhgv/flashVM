/* Sweep new/new[] failures through current-global boxing and both fallback policies. */
#define main native_instance_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "primitive_boxing_fixture.hpp"


static FVM_Result boxing_operation(BoxingFixture &f, FVM_Object *target, unsigned route, FVM_Value *out)
{
    if (route == 0)
    {
        FVM_Object *instance = (FVM_Object *)1;
        FVM_Result result = fvm_construct(f.context, "Object", 1, &f.input, &instance);
        if (!(result == FVM_OK ? instance != 0 : instance == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(out);
        if (result == FVM_OK)
        {
            *out = class_value(instance);
        }
        return result;
    }
    if (route == 1)
    {
        return fvm_function_invoke(f.context, target, 0, 1, &f.input, out);
    }
    FVM_Value args[2];
    args[0] = f.input;
    fvm_value_undefined(args + 1);
    args[1].type = FVM_VALUE_NULL;
    return fvm_object_call(f.context, target, route == 2 ? "call" : "apply", route == 2 ? 1 : 2, args, out);
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
    for (unsigned version = 6; version <= 8; version += 2)
    {
        for (unsigned type = 0; type < 3; ++type)
        {
            for (unsigned custom = 0; custom < 2; ++custom)
            {
                for (unsigned route = 0; route < 4; ++route)
                {
                    bool failed = false, success = false;
                    for (long boundary = 0; boundary < 1000 && !success; ++boundary)
                    {
                        BoxingFixture f;
                        boxing_init(runtime, f, version, type, custom ? 5 : 1);
                        FVM_Context *c = f.context;
                        FVM_Object *target = 0;
                        if (route < 2)
                        {
                            FVM_Value klass;
                            if (!(fvm_get_global(c, "Object", &klass) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            target = klass.object;
                        }
                        else
                        {
                            Program body;
                            body.variable("this");
                            body.finish();
                            if (!(fvm_function_create_avm1(
                                      c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &target) == FVM_OK &&
                                  fvm_gc_pin(c, target) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Value method;
                            if (!(fvm_object_get(c, target, route == 2 ? "call" : "apply", &method) == FVM_OK &&
                                  fvm_gc_pin(c, method.object) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        FVM_Value out;
                        fvm_value_number(&out, 91);
                        allowance = boundary;
                        FVM_Result status = boxing_operation(f, target, route, &out);
                        allowance = -1;
                        if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending &&
                              c->lexical_scopes.empty()))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (status == FVM_OUT_OF_MEMORY)
                        {
                            failed = true;
                            if (!(out.type == FVM_VALUE_UNDEFINED && fvm_gc_collect(c, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(boxing_operation(f, target, route, &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(status == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            success = true;
                        }
                        if (!(out.type == FVM_VALUE_OBJECT && fvm_gc_pin(c, out.object) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        boxing_check(f, out.object, route >= 2);
                        if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_context_destroy(c);
                    }
                    if (!(failed && success))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
