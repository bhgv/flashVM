/* Pending and active GC roots survive self/other cancellation, registration, reentry and errors. */
#include <stdio.h>
#include <stdlib.h>
#include "timers_fixture.hpp"


struct DeliveryState
{
    unsigned calls;
    FVM_Value own, other, spawn;
    bool mutate;
    FVM_Result result;


    DeliveryState() : calls(0), mutate(false), result(FVM_OK)
    {
    }
};


static FVM_Result body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    DeliveryState *s = (DeliveryState *)raw;
    ++s->calls;
    if (!(frame->argc == 2 && frame->argv[0].type == FVM_VALUE_OBJECT && frame->argv[1].type == FVM_VALUE_STRING))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (s->mutate)
    {
        timer_call(c, "clearInterval", 1, &s->own);
        timer_call(c, "clearInterval", 1, &s->other);
        timer_sample += 100;
        FVM_Value a[] = {s->spawn, timer_number(1)};
        timer_call(c, "setTimeout", 2, a);
        unsigned nested = 77;
        if (!(fvm_timers_poll(c, &nested) == FVM_INVALID_ARGUMENT && nested == 77))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
        if (!(c->timers_dispatching && !c->destroying))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value v;
    if (!(fvm_object_get(c, frame->argv[0].object, "retained", &v) == FVM_OK && v.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char expected[] = {'a', 0, 'b'};
    if (!(fvm_value_string_size(&frame->argv[1]) == 3 && !std::memcmp(frame->argv[1].string, expected, 3)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    if (s->result == FVM_EXCEPTION)
    {
        FVM_Value thrown = timer_number(99);
        if (!(fvm_exception_raise(c, &thrown) == FVM_EXCEPTION))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return s->result;
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
    for (unsigned repeating = 0; repeating < 2; ++repeating)
    {
        for (unsigned failure = 0; failure < 3; ++failure)
        {
            FVM_Context *c = timer_context(rt);
            if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            DeliveryState state;
            state.mutate = true;
            state.result = failure == 0 ? FVM_OK : failure == 1 ? FVM_ERROR : FVM_EXCEPTION;
            TimerProbe spawned, skipped;
            state.spawn = timer_function(c, &spawned);
            if (!(fvm_gc_pin(c, state.spawn.object) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *fn = 0, *arg = 0;
            const unsigned char empty[] = {0};
            if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK &&
                  fvm_function_set_body(c, fn, body, &state, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_create(c, &arg) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value retained = timer_number(42);
            if (!(fvm_object_set(c, arg, "retained", &retained) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            char borrowed[] = {'a', 0, 'b'};
            FVM_Value values[] = {timer_object(fn), timer_number(5), timer_object(arg), timer_number(0)};
            if (!(fvm_value_string_bytes(&values[3], borrowed, sizeof(borrowed)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            state.own = timer_call(c, repeating ? "setInterval" : "setTimeout", 4, values);
            std::memset(borrowed, 'x', sizeof(borrowed));
            FVM_Value later[] = {timer_function(c, &skipped), timer_number(5)};
            state.other = timer_call(c, "setTimeout", 2, later);
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(timer_poll(c, 1005, state.result) == 1 && state.calls == 1 && !skipped.calls && !spawned.calls))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (state.result == FVM_EXCEPTION)
            {
                if (!(fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(c->scheduled_calls.size() == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(timer_poll(c, 1106) == 1 && spawned.calls == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(c->scheduled_calls.empty()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_unpin(c, state.spawn.object) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
