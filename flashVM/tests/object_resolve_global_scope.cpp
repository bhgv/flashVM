/* Bare global variables skip __resolve; explicit member reads still invoke it. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/execution.h"

static unsigned scope_resolve_calls;


static FVM_Result
global_resolver(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *args, FVM_Value *out, void *)
{
    ++scope_resolve_calls;
    if (!(n == 1 && args[0].type == FVM_VALUE_STRING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 71);
    return FVM_OK;
}


static FVM_Result registered_binding(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    fvm_value_number(out, 93);
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
        FVM_Object *resolver = 0;
        if (!(fvm_function_create_native(c, global_resolver, 0, 0, &resolver) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value = class_value(resolver), name, out;
        if (!(fvm_set_global(c, "__resolve", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_register_function(c, "registered", registered_binding, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        scope_resolve_calls = 0;
        fvm_value_string(&name, "missing");
        int present = 99;
        FVM_Object *receiver = 0;
        if (!(fvm_exec_variable_lookup(c, 0, 0, 0, 0, 0, &name, &out, &receiver, &present) == FVM_OK && !present &&
              out.type == FVM_VALUE_UNDEFINED && !scope_resolve_calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_get_variable(c, 0, 0, 0, 0, &name, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED &&
              !scope_resolve_calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_call_function(c, 0, 0, 0, 0, 0, &name, 0, 0, &out) == FVM_OK &&
              out.type == FVM_VALUE_UNDEFINED && !scope_resolve_calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "missing", &out) == FVM_OK && out.number == 71 && scope_resolve_calls == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        scope_resolve_calls = 0;
        fvm_value_string(&name, "registered");
        if (!(fvm_exec_call_function(c, 0, 0, 0, 0, 0, &name, 0, 0, &out) == FVM_OK && out.number == 93 &&
              !scope_resolve_calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *parent = 0;
        if (!(fvm_object_create(c, &parent) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        if (!(fvm_object_set(c, parent, "registered", &value) == FVM_OK &&
              fvm_object_set_prototype(c, c->global, parent) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_variable_lookup(c, 0, 0, 0, 0, 0, &name, &out, &receiver, &present) == FVM_OK && present &&
              out.type == FVM_VALUE_UNDEFINED && !scope_resolve_calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (fvm_jit_available())
        {
            for (class_mode = 0; class_mode < 3; ++class_mode)
            {
                Program code;
                code.variable("missing");
                code.finish();
                out = class_run(c, code);
                if (!(out.type == FVM_VALUE_UNDEFINED && !scope_resolve_calls))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program shadow;
                shadow.variable("registered");
                shadow.finish();
                out = class_run(c, shadow);
                if (!(out.type == FVM_VALUE_UNDEFINED && !scope_resolve_calls))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program call;
                call.number(0);
                call.text("missing");
                call.op(0x3d);
                call.finish();
                out = class_run(c, call);
                if (!(out.type == FVM_VALUE_UNDEFINED && !scope_resolve_calls))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (!(!c->resolve_depth && !c->gc_frames && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
