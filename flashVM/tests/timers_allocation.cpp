/* Registration and poll OOM preserve pending records until their documented commit boundaries. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "timers_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned operation = 0; operation < 3; ++operation)
    {
        bool success = false;
        unsigned failures = 0;
        for (long boundary = 0; boundary < 512 && !success; ++boundary)
        {
            FVM_Context *c = timer_context(rt);
            TimerProbe probe;
            FVM_Value args[] = {timer_function(c, &probe), timer_number(1), timer_number(0)};
            if (!(fvm_value_string(&args[2], "borrowed timer argument") == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value out;
            if (operation)
            {
                timer_call(c, operation == 1 ? "setTimeout" : "setInterval", 3, args);
            }
            timer_sample = 1001;
            unsigned delivered = 777;
            allowance = boundary;
            FVM_Result r = operation ? fvm_timers_poll(c, &delivered) : fvm_call(c, "setTimeout", 3, args, &out);
            allowance = -1;
            if (!(!c->gc_frames && !c->gc_native_depth && !c->timers_dispatching && !c->avm1_execution_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (r == FVM_OK)
            {
                success = true;
                if (operation)
                {
                    if (!(delivered == 1 && probe.calls == 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(out.type == FVM_VALUE_NUMBER && c->scheduled_calls.size() == 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
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
                if (!operation)
                {
                    if (!(c->scheduled_calls.empty() && !c->last_timer_id && out.type == FVM_VALUE_UNDEFINED))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else if (!delivered)
                {
                    if (!(c->scheduled_calls.size() == 1 && !probe.calls))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (!(delivered == 1 && !probe.calls && c->scheduled_calls.size() == (operation == 1 ? 0U : 1U)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (operation && r != FVM_OK && !delivered)
            {
                if (!(timer_poll(c, 1001) == 1 && probe.calls == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_context_destroy(c);
        }
        if (!(success && failures))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_runtime_destroy(rt);
}
