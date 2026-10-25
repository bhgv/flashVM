/* KeyPress host scopes retain input state/redraw and restore all guards across callback failures. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_keypress_fixture.hpp"

static FVM_Object *selected; /* Pinned root used to test both recursive tree entry points. */
static unsigned calls;       /* Committed retained native callback entries. */
static unsigned mode;        /* Normal/error/exception/native-throw policy for one fresh context. */


/* A real retained AVM1 callback observes the outer scope and cannot collect or destroy its owner. */
static FVM_Result receive(FVM_Context *c, FVM_Object *, unsigned count, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(!count && c->clip_input_dispatching && c->stage_update_allowed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_dispatch_key_press(c, selected, 1) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_dispatch_input(c, selected, 64) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long reclaimed = 77;
    if (!(fvm_gc_step(c, &reclaimed) == FVM_OK && !reclaimed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    if (!(c->global))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++calls;
    if (!(fvm_call(c, "updateAfterEvent", 0, 0, out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (mode == 1)
    {
        return FVM_ERROR;
    }
    if (mode == 2)
    {
        FVM_Value value;
        fvm_value_number(&value, 99);
        return fvm_exception_raise(c, &value);
    }
    if (mode == 3)
    {
        throw std::bad_alloc();
    }
    if (mode == 4)
    {
        throw 42;
    }
    return FVM_OK;
}


/* Preserve old selected-record eligibility while granting it for the new host KeyPress route. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (mode = 0; mode < 5; ++mode)
    {
        FVM_Context *c = clip_context(runtime);
        selected = keypress_root(c, 8, 1, keypress_call("receiveKey"));
        if (!(fvm_register_function(c, "receiveKey", receive, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        calls = 0;
        FVM_Result expected = mode == 0   ? FVM_OK
                              : mode == 2 ? FVM_EXCEPTION
                              : mode == 3 ? FVM_OUT_OF_MEMORY
                                          : FVM_ERROR;
        if (!(fvm_movieclip_dispatch_key_press(c, selected, 1) == expected && calls == 1 && c->stage_update_pending))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        input_clean(c);
        if (mode == 2)
        {
            if (!(fvm_exception_clear(c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_movieclip_dispatch_key_press(c, selected, 2) == FVM_OK && calls == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned phase = 0; phase < 4; ++phase)
        {
            if (!phase)
            {
                c->destroying = true;
            }
            else if (phase == 1)
            {
                c->gc_collecting = true;
            }
            else if (phase == 2)
            {
                c->gc_releasing = true;
            }
            else
            {
                c->movieclip_sound_busy = true;
            }
            if (!(fvm_movieclip_dispatch_key_press(c, selected, 1) == FVM_INVALID_ARGUMENT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            c->destroying = c->gc_collecting = c->gc_releasing = c->movieclip_sound_busy = false;
        }
        input_clean(c);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime);
}
