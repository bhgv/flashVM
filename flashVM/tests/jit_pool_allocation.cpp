/* Pool compilation/adoption allocation failures restore transient snapshots and collector frames. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "../src/jit/internal.hpp"
#include "../src/standard/function_state.hpp"


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    const unsigned char body[] = {0x88, 4, 0, 1, 0, 'x', 0, 0x96, 2, 0, 8, 0, 0x96, 3, 0, 0, '!', 0, 0x47, 0x3e, 0};
    FVM_JitCode *code = 0;
    bool success = false;
    unsigned failures = 0;
    for (long boundary = 0; boundary < 2000 && !success; ++boundary)
    {
        allowance = boundary;
        FVM_Result status = fvm_jit_compile(body, sizeof(body), &code);
        allowance = -1;
        if (status == FVM_OK)
        {
            success = true;
        }
        else
        {
            if (!(status == FVM_OUT_OF_MEMORY && !code))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ++failures;
        }
    }
    if (!(success && failures && fvm_jit_destroy(code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
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
        FVM_Object *fn = 0;
        FVM_Value out;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
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
        if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_ScriptFunction definition;
        definition.body.assign(body, body + sizeof(body));
        definition.constants.push_back(fvm_detail::string("captured"));
        if (!(fvm_function_make_script(c, definition, 0, &fn) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_jit_compile(body, sizeof(body), &code) == FVM_OK &&
              fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        success = false;
        failures = 0;
        for (long boundary = 0; boundary < 2000 && !success; ++boundary)
        {
            FVM_GCRootFrame *roots = c->gc_frames;
            allowance = boundary;
            FVM_Result status = fvm_function_invoke(c, fn, 0, 0, 0, &out);
            allowance = -1;
            if (!(c->gc_frames == roots && !c->execution_mode.native_pool && !c->gc_native_depth && !c->script_depth &&
                  !c->avm1_execution_depth && !code->active))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (status == FVM_OK)
            {
                if (!(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "x!")))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                success = true;
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
        if (!(success && failures && fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK &&
              fvm_jit_destroy(code) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
