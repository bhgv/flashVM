/* Exact byte/capture/delivery ceilings preserve preparation, committed delivery and outer nesting. */
#include <stdio.h>
#include <stdlib.h>
#include "sound_snapshot_fixture.hpp"
#include "../src/core/execution_work_scope.hpp"
#include <limits>


struct SoundWork
{
    unsigned calls;
};


static FVM_Result charge_sound(FVM_Context *c, FVM_Object *, size_t, void *raw)
{
    SoundWork *p = static_cast<SoundWork *>(raw);
    ++p->calls;


    FVM_ExecutionWorkScope work(c);
    for (unsigned i = 0; i < 5; ++i)
    {
        FVM_Result result = work.step();
        if (result != FVM_OK)
        {
            return result;
        }
    }
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t source_bytes = sound_movie().size();
    for (unsigned long limit = 0; limit <= source_bytes + 29; ++limit)
    {
        rt->config.instruction_limit = 0;
        FVM_Context *c = tick_context(rt);
        SoundSnapshot p;
        FVM_Object *root = sound_snapshot_root(c, p);
        rt->config.instruction_limit = limit;
        unsigned long required = (unsigned long)c->swf_bytes.size() + 28;
        if (!(sound_snapshot_run(c, root) == (limit && limit < required ? FVM_ERROR : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->avm1_steps == (limit && limit < required ? limit : required)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(p.count == (!limit || limit >= required ? 2U : limit == required - 1 ? 1U : 0U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        if (!(!c->movieclip_sound_busy))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    rt->config.instruction_limit = 0;
    FVM_Context *c = tick_context(rt);
    SoundSnapshot p;
    FVM_Object *root = sound_snapshot_root(c, p);
    SoundWork callbacks = {0};
    if (!(fvm_movieclip_set_sound_callback(c, charge_sound, &callbacks) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.instruction_limit = (unsigned long)c->swf_bytes.size() + 32;
    if (!(sound_snapshot_run(c, root) == FVM_ERROR && callbacks.calls == 1 &&
          c->avm1_steps == rt->config.instruction_limit))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick_clean(c);
    if (!(!c->movieclip_sound_busy))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.instruction_limit = 0;
    c->avm1_execution_depth = 1;
    c->avm1_steps = std::numeric_limits<unsigned long>::max();
    if (!(sound_snapshot_run(c, root) == FVM_ERROR && c->avm1_steps == std::numeric_limits<unsigned long>::max() &&
          c->avm1_execution_depth == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->avm1_execution_depth = 0;
    tick_clean(c);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
