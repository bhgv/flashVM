/* Real OS workers stop cooperatively during input preparation and between native recipients. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "../src/core/execution_work_constants.h"
#include "flashvm/worker.h"
#include "flashvm/keyboard.h"
#include "flashvm/mouse.h"
#include "flashvm/input.h"
#include "flashvm/broadcaster.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"


struct Trace
{
    unsigned calls;
    bool stop_on_first;
};


static FVM_Value object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = o ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    v.object = o;
    return v;
}


static FVM_Result notify(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    Trace *trace = (Trace *)raw;
    ++trace->calls;
    if (trace->stop_on_first && trace->calls == 1)
    {
        FVM_Worker *self = 0;
        if (!(fvm_worker_current(c, &self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_worker_stop(self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result event(FVM_Context *c, unsigned kind, FVM_Object *source)
{
    if (!kind)
    {
        FVM_KeyboardState state = c->keyboard;
        state.down[FVM_KEY_SPACE] = 1;
        return fvm_input_dispatch_keyboard(c, &state, 1);
    }
    if (kind == 1)
    {
        return fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_MOVE, 19, 29);
    }
    if (kind == 2)
    {
        return fvm_input_dispatch_mouse_wheel(c, 1, 19, 29);
    }
    FVM_Value arg, out;
    if (kind == 3)
    {
        arg = object(0);
    }
    else
    {
        if (!(fvm_value_string(&arg, "onEvent") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return fvm_object_call(c, source, kind == 3 ? "setFocus" : "broadcastMessage", 1, &arg, &out);
}


static FVM_Result run(FVM_Context *c, const FVM_Value *input, FVM_Value *out, void *)
{
    const unsigned kind = (unsigned)input->number % 5;
    const bool midway = input->number >= 5;
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_keyboard_install(c) == FVM_OK && fvm_standard_mouse_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_selection_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *source = 0, *clip = 0, *candidate = 0;
    if (kind == 3)
    {
        FVM_Value ns;
        if (!(fvm_get_global(c, "Selection", &ns) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        source = ns.object;
        if (!(fvm_movieclip_create(c, "oldFocus", 1, &clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_input_set_keyboard_focus(c, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else if (kind == 4)
    {
        if (!(fvm_object_create(c, &source) == FVM_OK && fvm_gc_pin(c, source) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_broadcaster_initialize(c, source) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Trace trace;
    trace.calls = 0;
    trace.stop_on_first = midway;
    const unsigned count = midway ? 70 : 3;
    const char *handler = !kind       ? "onKeyDown"
                          : kind == 1 ? "onMouseMove"
                          : kind == 2 ? "onMouseWheel"
                          : kind == 3 ? "onSetFocus"
                                      : "onEvent";
    for (unsigned i = 0; i < count; ++i)
    {
        FVM_Object *listener = 0;
        if (!(fvm_object_create(c, &listener) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, listener, handler, notify, &trace, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        listener->methods[handler].collector_entry = true;
        if (!kind)
        {
            if (!(fvm_input_add_key_listener(c, listener) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (kind < 3)
        {
            if (!(fvm_input_add_mouse_listener(c, listener) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            FVM_Value arg = object(listener), result;
            if (!(fvm_object_call(c, source, "addListener", 1, &arg, &result) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        candidate = listener;
    }
    if (!midway)
    {
        FVM_Worker *self = 0;
        if (!(fvm_worker_current(c, &self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_worker_stop(self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int removed = 77;
        if (!kind)
        {
            if (!(fvm_input_add_key_listener(c, candidate) == FVM_ERROR))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_remove_key_listener(c, candidate, &removed) == FVM_ERROR && removed == 77))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (kind < 3)
        {
            if (!(fvm_input_add_mouse_listener(c, candidate) == FVM_ERROR))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_input_remove_mouse_listener(c, candidate, &removed) == FVM_ERROR && removed == 77))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            FVM_Value arg = object(candidate), result;
            if (!(fvm_object_call(c, source, "addListener", 1, &arg, &result) == FVM_ERROR))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_call(c, source, "removeListener", 1, &arg, &result) == FVM_ERROR))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(event(c, kind, source) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->avm1_execution_depth && !c->gc_frames && !c->gc_native_depth && !c->keyboard_dispatching &&
          !c->mouse_dispatching))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (midway)
    {
        if (!(trace.calls > 0 && trace.calls < count))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!((c->avm1_steps & FVM_EXECUTION_WORK_CANCEL_MASK) == FVM_EXECUTION_WORK_CANCEL_PHASE))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!kind)
        {
            if (!(c->keyboard.down[FVM_KEY_SPACE]))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (kind < 3)
        {
            double x, y;
            if (!(fvm_input_get_pointer(c, &x, &y) == FVM_OK && x == 19 && y == 29))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (kind == 3)
        {
            if (!(!c->keyboard_focus))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        if (!(!trace.calls && !c->keyboard.down[FVM_KEY_SPACE]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (kind == 3)
        {
            if (!(c->keyboard_focus == clip))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        double x, y;
        if (!(fvm_input_get_pointer(c, &x, &y) == FVM_OK && x == 0 && y == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


int main()
{
    FVM_Config config;
    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    config.instruction_limit = 0; /* Cancellation remains active with the numeric limit disabled. */
    for (unsigned i = 0; i < 10; ++i)
    {
        FVM_Value input, result;
        fvm_value_number(&input, i);
        FVM_Worker *worker = 0;
        if (!(fvm_worker_run(&config, run, &input, 0, &worker) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_worker_join(worker, &result) == FVM_OK && result.type == FVM_VALUE_BOOLEAN && result.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(worker);
    }
}
