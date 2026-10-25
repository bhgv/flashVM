/* Captured suffixes survive replacement/append/reentry; failed entries commit their attempt cursor once. */
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
    for (unsigned mutation = 1; mutation <= 4; ++mutation)
    {
        FVM_Context *c = tick_context(rt);
        FVM_Object *root = init_root(c, 8, 3);
        InitProbe p;
        init_bind(c, root, p);
        p.mutation = mutation;
        if (mutation == 4)
        {
            fvm_value_number(&p.table->indexed[1], 17);
        }
        if (!(fvm_movieclip_advance(c, root, 1) == (mutation == 4 ? FVM_INVALID_ARGUMENT : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        init_clean(c, p);
        if (!(p.calls[0] == 1 && p.calls[1] == (mutation == 4 ? 0U : 1U) && p.calls[2] == (mutation == 4 ? 0U : 1U) &&
              !p.calls[3]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        init_clean(c, p);
        if (!(p.calls[0] == 1 && p.calls[1] == (mutation == 4 ? 0U : 1U) && p.calls[2] == 1 &&
              p.calls[3] == (mutation == 2 ? 1U : 0U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(p.library->swf_initializer_next == (mutation == 2 ? 4UL : 3UL)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = 2;
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && c->avm1_steps == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = 0;
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
