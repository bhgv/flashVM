/* Native frame waits skip complete source Actions and preserve exact early-exit budgets. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"


static void wait_direct(Program &p, unsigned frame, unsigned skip)
{
    p.op(0x8a);
    p.op(3);
    p.op(0);
    p.op((unsigned char)frame);
    p.op((unsigned char)(frame >> 8));
    p.op((unsigned char)skip);
}


static void wait_value(Program &p, unsigned skip)
{
    p.op(0x8d);
    p.op(1);
    p.op(0);
    p.op((unsigned char)skip);
}
#ifndef FVM_WAIT_FIXTURE_ONLY


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
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (class_mode = 0; class_mode < 3; ++class_mode)
        {
            if (version && version < 4)
            {
                continue;
            }
            FVM_Context *c = 0;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            for (unsigned dynamic = 0; dynamic < 2; ++dynamic)
            {
                for (unsigned ready = 0; ready < 2; ++ready)
                {
                    for (unsigned skip = 0; skip <= 4; ++skip)
                    {
                        Program p;
                        p.number(77);
                        if (dynamic)
                        {
                            p.number(ready ? 16001 : 16002);
                            wait_value(p, skip == 4 ? 255 : skip);
                        }
                        else
                        {
                            wait_direct(p, ready ? 16000 : 16001, skip == 4 ? 255 : skip);
                        }
                        p.number(11);
                        p.finish();
                        FVM_Value out = class_run(c, p);
                        if (ready || !skip)
                        {
                            if (!(out.type == FVM_VALUE_NUMBER && out.number == 11))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (skip == 1)
                        {
                            if (!(out.type == FVM_VALUE_NUMBER && out.number == 77))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(out.type == FVM_VALUE_UNDEFINED))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                }
            }
            const char *values[] = {"16002", "1.6002e4", " 16002", "label"};
            for (unsigned i = 0; i < 4; ++i)
            {
                Program p;
                p.number(77);
                p.text(values[i]);
                wait_value(p, 1);
                p.number(11);
                p.finish();
                FVM_Value out = class_run(c, p);
                if (!(out.type == FVM_VALUE_NUMBER && out.number == (i < 2 ? 77 : 11)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            Program empty;
            wait_value(empty, 255);
            empty.number(11);
            empty.finish();
            FVM_Value out = class_run(c, empty, version ? FVM_OK : FVM_ERROR);
            if (version)
            {
                if (!(out.type == FVM_VALUE_NUMBER && out.number == 11))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            /* Multiple waits use distinct source-record skips, not the byte lengths of Push payloads. */
            Program chained;
            chained.number(77);
            wait_direct(chained, 16001, 1);
            chained.text("long skipped literal");
            wait_direct(chained, 16000, 255);
            chained.finish();
            out = class_run(c, chained);
            if (!(out.type == FVM_VALUE_NUMBER && out.number == 77))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    cfg.instruction_limit = 1;
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (class_mode = 0; class_mode < 3; ++class_mode)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Program exit_program;
        wait_direct(exit_program, 16001, 255);
        exit_program.op(0);
        FVM_Value out = class_run(c, exit_program);
        if (!(out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Program charged;
        wait_direct(charged, 16000, 0);
        charged.op(0);
        class_run(c, charged, FVM_ERROR);
        fvm_context_destroy(c);
        const unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (class_mode != 1)
        {
            FVM_ExecutionOptions options;
            if (!(fvm_execution_options_init(&options) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            options.mode = class_mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER;
            unsigned long entries = c->execution_mode.stats.native_entries;
            Program implicit;
            wait_direct(implicit, 16000, 0);
            if (!(fvm_avm1_execute_options(c, implicit.bytes.begin(), implicit.bytes.size(), &options) == FVM_ERROR))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program implicit_exit;
            wait_direct(implicit_exit, 16001, 255);
            if (!(fvm_avm1_execute_options(c, implicit_exit.bytes.begin(), implicit_exit.bytes.size(), &options) ==
                  FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->execution_mode.stats.native_entries == entries + (class_mode == 2 ? 2 : 0)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
