/* Budget exhaustion while copying/entering init suffixes preserves the exact at-most-once cursor. */
#include <stdio.h>
#include <stdlib.h>
#include "initializer_progress_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned long limit = 1; limit <= 26; ++limit)
    {
        FVM_Context *c = tick_context(rt);
        FVM_Object *root = init_root(c, 8, 3);
        InitProbe p;
        init_bind(c, root, p);
        rt->config.instruction_limit = limit;
        FVM_Result status = fvm_movieclip_advance(c, root, 1);
        if (!(status == (limit < 26 ? FVM_ERROR : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        init_clean(c, p);
        if (!(p.library->swf_initializer_next <= 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 3; ++i)
        {
            if (!(p.calls[i] <= 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        unsigned long cursor = p.library->swf_initializer_next;
        if (limit <= 4)
        {
            if (!(!cursor && !p.calls[0]))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        rt->config.instruction_limit = 0;
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && p.library->swf_initializer_next == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 3; ++i)
        {
            if (!(p.calls[i] <= 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        init_clean(c, p);
        fvm_context_destroy(c);
    }
    /* A consumed prefix is neither copied nor revalidated; only pending ordinals consume work. */
    FVM_Context *c = tick_context(rt);
    FVM_Object *root = init_root(c, 8, 3);
    InitProbe p;
    init_bind(c, root, p);
    p.library->swf_initializer_next = 2;
    fvm_value_number(&p.table->indexed[0], 17);
    rt->config.instruction_limit = 10;
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && c->avm1_steps == 10))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!p.calls[0] && !p.calls[1] && p.calls[2] == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    init_clean(c, p);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
