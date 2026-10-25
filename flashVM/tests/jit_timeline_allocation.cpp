/* Timeline allocation failures restore invocation roots while successful effects preserve stack values. */
#define main timeline_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#define FVM_TIMELINE_FIXTURE_ONLY
#include "jit_timeline.cpp"


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
    const unsigned char commands[] = {0x04, 0x05, 0x06, 0x07};
    const unsigned frames[] = {3, 1, 2, 2, 4, 2, 3};
    for (unsigned loaded = 0; loaded < 2; ++loaded)
    {
        for (unsigned which = 0; which < 7; ++which)
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
            FVM_Object *root = timeline_setup(c);
            Program p;
            p.number(31);
            if (which < 4)
            {
                p.op(commands[which]);
            }
            else if (which == 4)
            {
                timeline_direct(p, 3);
            }
            else if (which == 5)
            {
                timeline_label(p, "two");
            }
            else
            {
                p.text("002");
                timeline_dynamic(p, 3, 1);
            }
            p.finish();
            FVM_Object *fn = 0;
            FVM_JitCode *code = 0;
            FVM_Value out, initial;
            fvm_value_number(&initial, 2);
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
            bool compiled = false;
            unsigned compile_failures = 0;
            for (long boundary = 0; boundary < 2000 && !compiled; ++boundary)
            {
                FVM_JitCode *candidate = code;
                allowance = boundary;
                FVM_Result status = fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &candidate);
                allowance = -1;
                if (status == FVM_OK)
                {
                    compiled = true;
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
                    ++compile_failures;
                }
            }
            if (!(compiled && compile_failures))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            bool success = false;
            unsigned failures = 0;
            for (long boundary = 0; boundary < 2000 && !success; ++boundary)
            {
                if (!(fvm_exec_frame_control(c, root, FVM_EXEC_GOTO_FRAME, &initial, 1) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_GCRootFrame *roots = c->gc_frames;
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
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 31))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    timeline_state(root, frames[which], which == 2 || which == 6);
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
            if (!(success && failures && fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK &&
                  fvm_jit_destroy(code) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            timeline_state(root, frames[which], which == 2 || which == 6);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
