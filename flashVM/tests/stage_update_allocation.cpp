/* Host state operations allocate nothing; delivery OOM always restores dynamic eligibility. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "timers_fixture.hpp"
#include "flashvm/stage.h"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Context *c = timer_context(rt);
    FVM_GCRootFrame *roots = c->gc_frames;
    size_t objects = c->objects.size(), strings = c->strings.size();
    allowance = 0;
    for (unsigned i = 0; i < 128; ++i)
    {
        int flag = 77;
        if (!(fvm_stage_request_update(c) == FVM_OK && fvm_stage_get_update(c, &flag) == FVM_OK && flag == 1 &&
              fvm_stage_take_update(c, &flag) == FVM_OK && flag == 1 && fvm_stage_take_update(c, &flag) == FVM_OK &&
              !flag))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    allowance = -1;
    if (!(c->gc_frames == roots && c->objects.size() == objects && c->strings.size() == strings))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    bool success = false;
    unsigned failures = 0;
    for (long boundary = 0; boundary < 256 && !success; ++boundary)
    {
        c = timer_context(rt);
        TimerProbe probe;
        FVM_Value args[] = {timer_function(c, &probe), timer_number(1)};
        timer_call(c, "setInterval", 2, args);
        timer_sample = 1001;
        unsigned delivered = 77;
        allowance = boundary;
        FVM_Result r = fvm_timers_poll(c, &delivered);
        allowance = -1;
        if (!(!c->stage_update_allowed && !c->gc_frames && !c->timers_dispatching && !c->avm1_execution_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (r == FVM_OK)
        {
            success = true;
            if (!(delivered == 1 && probe.calls == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(r == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ++failures;
        }
        fvm_context_destroy(c);
    }
    if (!(success && failures))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
}
