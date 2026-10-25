/* Availability, ID stability, delay/clock policy, dynamic method lookup and cancellation. */
#include <stdio.h>
#include <stdlib.h>
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        for (unsigned route = 0; route < 2; ++route)
        {
            FVM_Context *c = timer_context(rt, version);
            TimerProbe probe;
            FVM_Value args[] = {timer_function(c, &probe), timer_number(10)};
            const char *names[] = {"setInterval", "clearInterval", "setTimeout", "clearTimeout"};
            for (unsigned i = 0; i < 4; ++i)
            {
                FVM_Value fn;
                if (!(fvm_get_global(c, names[i], &fn) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_numeric_callback_id(fn.object->invocable.callback) == 164 + i))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_numeric_callback_from_id(164 + i) == fn.object->invocable.callback))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (version && version < 6)
            {
                timer_call(c, "setInterval", 2, args, FVM_UNSUPPORTED, route);
                timer_call(c, "clearInterval", 1, args, FVM_UNSUPPORTED, route);
                timer_call(c, "setTimeout", 2, args, FVM_UNSUPPORTED, route);
                timer_call(c, "clearTimeout", 1, args, FVM_UNSUPPORTED, route);
                if (!(c->scheduled_calls.empty()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
                continue;
            }
            FVM_Value id = timer_call(c, "setInterval", 2, args, FVM_OK, route);
            if (!(id.type == FVM_VALUE_NUMBER && id.number == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(timer_poll(c, 1009) == 0 && !probe.calls))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(timer_poll(c, 1010) == 1 && probe.calls == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(timer_poll(c, 2000) == 1 && probe.calls == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(timer_poll(c, 2000) == 0 && probe.calls == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(timer_poll(c, 1500) == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            timer_call(c, "clearInterval", 1, &id, FVM_OK, route);
            if (!(timer_poll(c, 3000) == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(timer_call(c, "clearInterval", 1, &id, FVM_OK, route).type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            timer_call(c, "clearInterval", 0, 0, FVM_OK, route);
            if (version && version < 8)
            {
                timer_call(c, "setTimeout", 2, args, FVM_UNSUPPORTED, route);
            }
            else
            {
                id = timer_call(c, "setTimeout", 2, args, FVM_OK, route);
                if (!(id.number == 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(timer_poll(c, 3010) == 1 && probe.calls == 3))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(timer_poll(c, 4010) == 0 && c->scheduled_calls.empty()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                id = timer_call(c, "setTimeout", 2, args, FVM_OK, route);
                timer_call(c, "clearTimeout", 1, &id, FVM_OK, route);
                if (!(timer_poll(c, 5010) == 0))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            double invalid[] = {0, -1, 0.5, HUGE_VAL, 2147483648.0, std::sqrt(-1.0)};
            for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
            {
                args[1] = timer_number(invalid[i]);
                timer_call(c, "setInterval", 2, args, FVM_UNSUPPORTED, route);
                if (!(c->scheduled_calls.empty()))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            args[1] = timer_number(10.9);
            id = timer_call(c, "setInterval", 2, args, FVM_OK, route);
            timer_available = 0;
            if (!(timer_poll(c, 6000, FVM_ERROR) == 0 && c->scheduled_calls.size() == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            timer_available = 1;
            timer_call(c, "clearInterval", 1, &id, FVM_OK, route);
            timer_available = 0;
            timer_call(c, "setInterval", 2, args, FVM_ERROR, route);
            timer_available = 1;
            if (!(c->scheduled_calls.empty()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    /* The object/method overload resolves the current method at each delivery and preserves this. */
    FVM_Context *c = timer_context(rt);
    TimerProbe old_probe, new_probe;
    FVM_Object *owner = 0;
    if (!(fvm_object_create(c, &owner) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value old_fn = timer_function(c, &old_probe), new_fn = timer_function(c, &new_probe);
    if (!(fvm_object_set(c, owner, "tick", &old_fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value args[] = {timer_object(owner), timer_number(0), timer_number(5)};
    if (!(fvm_value_string(&args[1], "tick") == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value id = timer_call(c, "setInterval", 3, args);
    if (!(fvm_object_set(c, owner, "tick", &new_fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(timer_poll(c, 1005) == 1 && !old_probe.calls && new_probe.calls == 1 && new_probe.receiver == owner))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    timer_call(c, "clearTimeout", 1, &id);
    if (!(c->scheduled_calls.empty()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* IDs do not wrap/reuse after the last representable published identifier. */
    c->last_timer_id = 4294967294UL;
    FVM_Value simple[] = {new_fn, timer_number(1)};
    id = timer_call(c, "setTimeout", 2, simple);
    if (!(id.number == 4294967295.0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    timer_call(c, "clearInterval", 1, &id);
    timer_call(c, "setTimeout", 2, simple, FVM_ERROR);
    if (!(c->scheduled_calls.empty()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned count = 77;
    if (!(fvm_timers_poll(0, &count) == FVM_INVALID_ARGUMENT && count == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_timers_poll(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_collecting = true;
    if (!(fvm_timers_poll(c, &count) == FVM_INVALID_ARGUMENT && count == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_collecting = false;
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
