/* Imported scheduling globals retain trusted collecting coercion but never copy pending queues. */
#include <stdio.h>
#include <stdlib.h>
#include "timers_fixture.hpp"
#include "flashvm/serialization.h"


struct Coercion
{
    unsigned calls;
    double result;


    Coercion() : calls(0), result(5)
    {
    }
};


static FVM_Result number_body(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *raw)
{
    Coercion *s = (Coercion *)raw;
    ++s->calls;
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, s->result);
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
    const char *names[] = {"setInterval", "clearInterval", "setTimeout", "clearTimeout"};
    for (unsigned index = 0; index < 4; ++index)
    {
        FVM_Context *a = timer_context(rt);
        FVM_Value source;
        if (!(fvm_get_global(a, names[index], &source) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value callback;
        if (!(fvm_get_global(a, "getTimer", &callback) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value pending[] = {callback, timer_number(1)};
        timer_call(a, "setInterval", 2, pending);
        unsigned char *bytes = 0;
        size_t size = 0;
        if (!(fvm_value_serialize(a, &source, &bytes, &size) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(a);
        FVM_Context *b = timer_context(rt);
        FVM_Value imported;
        if (!(fvm_value_deserialize(b, bytes, size, &imported) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(bytes);
        if (!(b->scheduled_calls.empty() && imported.object->invocable.collector_entry &&
              fvm_gc_pin(b, imported.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Coercion state;
        const unsigned char empty[] = {0};
        FVM_Object *convert = 0, *coercible = 0;
        if (!(fvm_function_create_avm1(b, empty, sizeof(empty), 0, 0, 0, &convert) == FVM_OK &&
              fvm_function_set_body(b, convert, number_body, &state, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(b, &coercible) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value = timer_object(convert);
        if (!(fvm_object_set(b, coercible, "valueOf", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        TimerProbe probe;
        FVM_Value args[] = {timer_function(b, &probe), timer_object(coercible)}, out;
        if (index == 0 || index == 2)
        {
            if (!(fvm_function_invoke(b, imported.object, 0, 2, args, &out) == FVM_OK && state.calls == 1 &&
                  out.number == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(timer_poll(b, 1005) == 1 && probe.calls == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            FVM_Value simple[] = {args[0], timer_number(5)};
            timer_call(b, "setTimeout", 2, simple);
            state.result = 1;
            if (!(fvm_function_invoke(b, imported.object, 0, 1, args + 1, &out) == FVM_OK && state.calls == 1 &&
                  b->scheduled_calls.empty()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(b);
    }
    fvm_runtime_destroy(rt);
}
