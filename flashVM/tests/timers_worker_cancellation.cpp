/* Real OS workers cancel before timer snapshot publication and during bounded delivery preparation. */
#include <stdio.h>
#include <stdlib.h>
#include "timers_fixture.hpp"
#include "flashvm/worker.h"


struct StopProbe
{
    unsigned calls;
    bool midway;


    StopProbe() : calls(0), midway(false)
    {
    }
};


static FVM_Result stop_callback(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    StopProbe *p = (StopProbe *)raw;
    ++p->calls;
    if (p->midway && p->calls == 1)
    {
        FVM_Worker *self = 0;
        if (!(fvm_worker_current(c, &self) == FVM_OK && fvm_worker_stop(self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned long wait = 777;
        int pending = 99;
        if (!(fvm_timers_next_delay(c, 1000, &wait, &pending) == FVM_ERROR && wait == 777 && pending == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result worker(FVM_Context *c, const FVM_Value *input, FVM_Value *out, void *)
{
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    StopProbe state;
    state.midway = input->boolean != 0;
    FVM_Object *fn = 0;
    if (!(fvm_function_create_native(c, stop_callback, &state, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value args[] = {timer_object(fn), timer_number(1)};
    unsigned count = state.midway ? 70 : 3;
    for (unsigned i = 0; i < count; ++i)
    {
        timer_call(c, "setTimeout", 2, args);
    }
    if (!state.midway)
    {
        FVM_Worker *self = 0;
        if (!(fvm_worker_current(c, &self) == FVM_OK && fvm_worker_stop(self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned long wait = 777;
        int pending = 99;
        if (!(fvm_timers_next_delay(c, 1000, &wait, &pending) == FVM_ERROR && wait == 777 && pending == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    unsigned delivered = timer_poll(c, 1001, FVM_ERROR);
    if (state.midway)
    {
        if (!(delivered > 0 && delivered < count && state.calls == delivered))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(!delivered && !state.calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(c->scheduled_calls.size() == count - delivered))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cfg.instruction_limit = 0;
    for (unsigned mode = 0; mode < 2; ++mode)
    {
        timer_sample = 1000;
        FVM_Value input, out;
        fvm_value_boolean(&input, mode != 0);
        FVM_Worker *w = 0;
        if (!(fvm_worker_run(&cfg, worker, &input, 0, &w) == FVM_OK && fvm_worker_join(w, &out) == FVM_OK &&
              out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(w);
    }
}
