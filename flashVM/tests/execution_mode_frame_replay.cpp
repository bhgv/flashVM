/* Bounded frame diagnostics preserve implicit End framing and the original thrown value. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution_mode.h"
#include "flashvm/crash.h"
#include "flashvm/standard.h"
#include "flashvm/exception.h"
#include "../src/avm1/block.hpp"
#include "../src/jit/internal.hpp"

static unsigned calls, ends, retry_throws;


static FVM_Result getter(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++calls;
    fvm_value_number(out, calls == 1 ? 111 : 42);
    return calls == 1 || retry_throws ? fvm_exception_raise(c, out) : FVM_OK;
}


static void logger(FVM_Context *, const FVM_ExecutionEvent *event, void *)
{
    if (event->kind == FVM_EXEC_EVENT_REPLAY_END)
    {
        ++ends;
        if (!(event->status == (retry_throws ? FVM_EXCEPTION : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}


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
        for (unsigned terminated = 0; terminated < 2; ++terminated)
        {
            for (unsigned retry = 0; retry < 2; ++retry)
            {
                for (retry_throws = 0; retry_throws < 2; ++retry_throws)
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
                    if (!(fvm_standard_install(c) == FVM_OK &&
                          fvm_object_define_accessor_rooted(c, c->global, "x", getter, 0, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_execution_set_mode(c, FVM_EXECUTION_JIT, retry ? FVM_EXEC_REPLAY_ON_EXCEPTION : 0) ==
                              FVM_OK &&
                          fvm_execution_set_logger(c, logger, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_crash_configure(c, FVM_CRASH_BACKTRACE, 0, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    const unsigned char body[] = {0x96, 3, 0, 0, 'x', 0, 0x1c, 0};
                    calls = ends = 0;
                    FVM_GCRootFrame *roots = c->gc_frames;
                    FVM_Value exception;
                    if (!(fvm_avm1_execute_block_receiver(c, body, sizeof(body) - (terminated ? 0 : 1), c->global) ==
                          FVM_EXCEPTION))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(calls == 1 + retry && ends == retry))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(c->gc_frames == roots && !c->avm1_execution_depth && !c->execution_mode.replay_depth &&
                          !c->script_depth && !c->gc_native_depth && c->avm1_steps == 2))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_exception_get(c, &exception) == FVM_OK && exception.type == FVM_VALUE_NUMBER &&
                          exception.number == 111))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_ExecutionStats stats;
                    stats.struct_size = sizeof(stats);
                    if (!(fvm_execution_get_stats(c, &stats) == FVM_OK && stats.native_entries == 1 &&
                          stats.interpreter_entries == retry && stats.replays == retry &&
                          stats.replay_steps == (retry_throws ? 2 : 3) * retry))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_CrashFrame crash;
                    if (!(fvm_crash_frame(c, 0, &crash) == FVM_OK && crash.jit &&
                          crash.bytecode_size == sizeof(body) - (terminated ? 0 : 1)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (retry)
                    {
                        if (!(stats.last_replay_status == (retry_throws ? FVM_EXCEPTION : FVM_OK)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    for (fvm_detail::map<fvm_detail::string, FVM_JitCode *>::iterator i =
                             c->execution_mode.cache.begin();
                         i != c->execution_mode.cache.end();
                         ++i)
                    {
                        if (!(!i->second || !i->second->active))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_exception_clear(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
