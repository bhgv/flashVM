/* Installation conflicts preserve exact/version-folded bindings; late loads enforce availability. */
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
    const char *names[] = {"setInterval", "clearInterval", "setTimeout", "clearTimeout"};
    const char *upper[] = {"SETINTERVAL", "CLEARINTERVAL", "SETTIMEOUT", "CLEARTIMEOUT"};
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (unsigned which = 0; which < 4; ++which)
        {
            for (unsigned mixed = 0; mixed < 2; ++mixed)
            {
                for (unsigned slot = 0; slot < 2; ++slot)
                {
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                    swf[3] = (unsigned char)version;
                    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    const char *name = mixed ? upper[which] : names[which];
                    FVM_Value v = timer_number(77);
                    if (!slot)
                    {
                        if (!(fvm_set_global(c, name, &v) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(fvm_register_function(c, name, timer_probe, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    FVM_Result expected = !mixed || version < 7 ? FVM_INVALID_ARGUMENT : FVM_OK;
                    if (!(fvm_standard_install(c) == expected))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!slot)
                    {
                        if (!(fvm_get_global(c, name, &v) == FVM_OK && v.type == FVM_VALUE_NUMBER && v.number == 77))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(c->functions[name].callback == timer_probe))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    for (unsigned version = 5; version <= 7; version += 2)
    {
        FVM_Context *c = timer_context(rt, 0);
        TimerProbe probe;
        FVM_Value args[] = {timer_function(c, &probe), timer_number(1)};
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 4; ++i)
        {
            if (version < (i < 2 ? 6U : 8U))
            {
                timer_call(c, names[i], 2, args, FVM_UNSUPPORTED);
            }
        }
        if (!(c->scheduled_calls.empty()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    FVM_Context *c = timer_context(rt);
    TimerProbe probe;
    FVM_Value args[] = {timer_function(c, &probe), timer_number(1)};
    for (unsigned phase = 0; phase < 3; ++phase)
    {
        if (phase == 0)
        {
            c->gc_collecting = true;
        }
        else if (phase == 1)
        {
            c->gc_releasing = true;
        }
        else
        {
            c->destroying = true;
        }
        for (unsigned i = 0; i < 4; ++i)
        {
            timer_call(c, names[i], 2, args, FVM_INVALID_ARGUMENT);
        }
        c->gc_collecting = c->gc_releasing = c->destroying = false;
        if (!(c->scheduled_calls.empty()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_detail::vector<FVM_Value> many(259);
    many[0] = args[0];
    many[1] = args[1];
    for (unsigned i = 2; i < 259; ++i)
    {
        many[i] = timer_number(i);
    }
    timer_call(c, "setTimeout", 259, many.begin(), FVM_UNSUPPORTED);
    if (!(c->scheduled_calls.empty() && !c->last_timer_id))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
