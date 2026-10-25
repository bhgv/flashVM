/* Entire event batches and callbacks share work; completed records survive errors and exceptions. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_actions_fixture.hpp"

static unsigned deliveries;
static FVM_Result result_status = FVM_OK;


static FVM_Result checkpoint(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++deliveries;
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long n = 77;
    if (!(fvm_gc_step(c, &n) == FVM_OK && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    if (result_status == FVM_EXCEPTION)
    {
        FVM_Value v;
        fvm_value_number(&v, 99);
        return fvm_exception_raise(c, &v);
    }
    return result_status;
}


static Bytes call_checkpoint()
{
    Bytes b;
    integer(b, 0);
    text(b, "checkpoint");
    b.push_back(0x3d);
    b.push_back(0x17);
    b.push_back(0);
    return b;
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
    bool completed = false;
    for (unsigned limit = 1; limit <= 256; ++limit)
    {
        FVM_Context *c = clip_context(rt);
        Bytes events;
        u16(events, 0);
        u32(events, 1);
        for (unsigned i = 0; i < 4; ++i)
        {
            clip_record(events, 8, 1, call_checkpoint());
        }
        u32(events, 0);
        rt->config.instruction_limit = 0;
        FVM_Object *root = clip_load(c, clip_movie(8, events));
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *actor = get(c, root, "actor").object;
        if (!(fvm_register_function(c, "checkpoint", checkpoint, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = limit;
        deliveries = 0;
        FVM_Result r = fvm_movieclip_dispatch_clip_actions(c, actor, 1, 0);
        if (!(r == FVM_OK || r == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->avm1_steps <= limit + 1 && deliveries <= 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (limit < 20)
        {
            if (!(r == FVM_ERROR && deliveries < 4))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (r == FVM_OK)
        {
            completed = true;
            if (!(deliveries == 4))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        clip_clean(c);
        fvm_context_destroy(c);
    }
    if (!(completed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.instruction_limit = 0;
    for (unsigned failure = 0; failure < 2; ++failure)
    {
        FVM_Context *c = clip_context(rt);
        Bytes events;
        u16(events, 0);
        u32(events, 1);
        clip_record(events, 8, 1, call_checkpoint());
        clip_record(events, 8, 1, action(17));
        u32(events, 0);
        FVM_Object *root = clip_load(c, clip_movie(8, events));
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *actor = get(c, root, "actor").object;
        if (!(fvm_register_function(c, "checkpoint", checkpoint, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        result_status = failure ? FVM_EXCEPTION : FVM_ERROR;
        deliveries = 0;
        if (!(fvm_movieclip_dispatch_clip_actions(c, actor, 1, 0) == result_status && deliveries == 1 &&
              get(c, actor, "seen").type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clip_clean(c);
        if (failure)
        {
            if (!(fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        result_status = FVM_OK;
        if (!(fvm_movieclip_dispatch_clip_actions(c, actor, 1, 0) == FVM_OK && deliveries == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, actor, "seen", 17);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
