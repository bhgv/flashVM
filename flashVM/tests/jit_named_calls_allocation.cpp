/* Inject invocation allocation failures across native named calls and construction. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#define FVM_METHOD_FIXTURE_ONLY
#include "jit_methods.cpp"


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned loaded = 0; loaded < 2; ++loaded)
    {
        for (unsigned construct = 0; construct < 2; ++construct)
        {
            FVM_Context *c = 0;
            FVM_Object *owner = 0, *method = 0, *fn = 0;
            FVM_Value value, arg, out;
            const unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0}, empty[] = {0};
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (loaded)
            {
                if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_standard_install(c) == FVM_OK && fvm_object_create(c, &owner) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(c, owner) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &method) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_set_body(c, method, called, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_undefined(&value);
            value.type = FVM_VALUE_OBJECT;
            value.object = method;
            if (!(fvm_object_set(c, c->global, "run", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program p;
            p.number(25);
            p.number(17);
            p.number(2);
            p.text("run");
            p.op(construct ? 0x40 : 0x3d);
            p.finish();
            const char *params[] = {"input"};
            if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 1, params, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(c, fn) == FVM_OK && fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &active) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_function_set_body(c, fn, fvm_jit_body, active, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            arg = value;
            arg.object = owner;
            bool success = false;
            unsigned failures = 0;
            for (long boundary = 0; boundary < 2000 && !success; ++boundary)
            {
                FVM_GCRootFrame *roots = c->gc_frames;
                allowance = boundary;
                FVM_Result status = fvm_function_invoke(c, fn, 0, 1, &arg, &out);
                allowance = -1;
                if (!(c->gc_frames == roots && !c->gc_native_depth && !c->script_depth && !c->avm1_execution_depth &&
                      !active->active))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (status == FVM_OK)
                {
                    success = true;
                    if (!(out.type == (construct ? FVM_VALUE_OBJECT : FVM_VALUE_NUMBER)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(status == FVM_OUT_OF_MEMORY))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    ++failures;
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(success && failures && fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_jit_destroy(active) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            active = 0;
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
