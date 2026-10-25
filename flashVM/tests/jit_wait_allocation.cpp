/* Frame-wait compile/invoke allocation failures retain handles, roots and native exit state. */
#define main wait_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#define FVM_WAIT_FIXTURE_ONLY
#include "jit_wait_frames.cpp"


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
        for (unsigned kind = 0; kind < 3; ++kind)
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
            Program p;
            p.number(77);
            if (kind)
            {
                p.text("16002");
                wait_value(p, kind == 2 ? 255 : 1);
            }
            else
            {
                wait_direct(p, 16001, 1);
            }
            p.number(11);
            p.finish();
            FVM_Object *fn = 0;
            FVM_JitCode *code = 0;
            FVM_Value out;
            if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK &&
                  fvm_gc_pin(c, fn) == FVM_OK && fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &code) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            bool success = false;
            unsigned failures = 0;
            for (long boundary = 0; boundary < 2000 && !success; ++boundary)
            {
                FVM_JitCode *candidate = code;
                allowance = boundary;
                FVM_Result status = fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &candidate);
                allowance = -1;
                if (status == FVM_OK)
                {
                    success = true;
                    if (!(candidate != code && fvm_jit_destroy(candidate) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(status == FVM_OUT_OF_MEMORY && candidate == code))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    ++failures;
                }
            }
            if (!(success && failures && fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
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
                if (!(c->gc_frames == roots && !c->script_depth && !c->gc_native_depth && !c->avm1_execution_depth &&
                      !code->active))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (status == FVM_OK)
                {
                    success = true;
                    if (!(kind == 2 ? out.type == FVM_VALUE_UNDEFINED
                                    : out.type == FVM_VALUE_NUMBER && out.number == 77))
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
            if (!(success && failures && fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK &&
                  fvm_jit_destroy(code) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
