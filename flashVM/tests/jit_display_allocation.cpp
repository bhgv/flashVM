/* Native display coercion, mutation and counted path publication restore roots at allocation failures. */
#define main display_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#define FVM_DISPLAY_CALLBACK_FIXTURE_ONLY
#include "jit_display_callbacks.cpp"


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
        FVM_Context *c = 0;
        const unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
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
        FVM_Object *child = 0, *root = display_setup(c, &child);
        unsigned ids[] = {1, 2, 3};
        display_coercion(c, "Selector", ids);
        display_coercion(c, "Path", ids + 1);
        display_coercion(c, "Scalar", ids + 2);
        Program p;
        p.variable("Path");
        p.variable("Selector");
        p.variable("Scalar");
        p.op(0x23);
        display_target(p);
        p.number(0);
        p.op(0x22);
        p.op(0x17);
        display_target(p);
        p.op(0x45);
        p.finish();
        FVM_Object *fn = 0;
        FVM_JitCode *code = 0;
        FVM_Value out;
        if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK &&
              fvm_gc_pin(c, fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &code) == FVM_OK &&
              fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        bool success = false;
        unsigned failures = 0;
        property_index = 0;
        fail_conversion = 0;
        for (long boundary = 0; boundary < 3000 && !success; ++boundary)
        {
            FVM_GCRootFrame *roots = c->gc_frames;
            conversion_count = 0;
            allowance = boundary;
            FVM_Result status = fvm_function_invoke(c, fn, root, 0, 0, &out);
            allowance = -1;
            if (!(c->gc_frames == roots && !c->script_depth && !c->gc_native_depth && !c->avm1_execution_depth &&
                  !code->active))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (status == FVM_OK)
            {
                success = true;
                if (!(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "_level0.child")))
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
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        FVM_GCRootScope *hold = 0;
        if (!(success && failures && fvm_gc_roots_begin(c, &hold) == FVM_OK && fvm_gc_roots_add(hold, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK &&
              fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!std::strcmp(out.string, "_level0.child") && fvm_gc_roots_end(hold) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
