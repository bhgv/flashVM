/* Shared native event budgets: preparation rollback, committed partial delivery and nested work. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "../src/core/execution_work_scope.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/mouse.h"
#include "flashvm/input.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"


struct Trace
{
    unsigned calls;
    bool busy;
};


static FVM_Value object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = o ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    v.object = o;
    return v;
}


static FVM_Result receive(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    Trace *t = (Trace *)raw;
    ++t->calls;
    if (!(c->avm1_execution_depth && !c->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(out);
    if (t->busy)
    {


        FVM_ExecutionWorkScope nested(c);
        for (unsigned i = 0; i < 3; ++i)
        {
            FVM_Result result = nested.step();
            if (result != FVM_OK)
            {
                return result;
            }
        }
    }
    return FVM_OK;
}


static FVM_Result deliver(FVM_Context *c, unsigned kind, FVM_Object *selection, const FVM_KeyboardState &state)
{
    if (!kind)
    {
        return fvm_input_dispatch_keyboard(c, &state, 1);
    }
    if (kind == 1)
    {
        return fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_DOWN, 23, 45);
    }
    if (kind == 2)
    {
        return fvm_input_dispatch_mouse_wheel(c, -2.5, 23, 45);
    }
    FVM_Value arg = object(0), out;
    return fvm_object_call(c, selection, "setFocus", 1, &arg, &out);
}


static void clean(FVM_Context *c)
{
    if (!(!c->avm1_execution_depth && !c->gc_frames && !c->gc_native_depth && !c->script_depth &&
          !c->mouse_dispatching && !c->keyboard_dispatching))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    const unsigned long ordinary = rt->config.instruction_limit;
    for (unsigned kind = 0; kind < 4; ++kind)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_selection_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value selection;
        if (!(fvm_get_global(c, "Selection", &selection) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *clip = 0;
        if (!(fvm_movieclip_create(c, "focused", 1, &clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, clip) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Trace t;
        t.calls = 0;
        t.busy = false;
        const char *handler = !kind       ? "onKeyDown"
                              : kind == 1 ? "onMouseDown"
                              : kind == 2 ? "onMouseWheel"
                                          : "onSetFocus";
        for (unsigned i = 0; i < 3; ++i)
        {
            FVM_Object *listener = 0;
            if (!(fvm_object_create(c, &listener) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_define_method_rooted(c, listener, handler, receive, &t, 0) == FVM_OK))
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
                FVM_Value arg = object(listener), out;
                if (!(fvm_object_call(c, selection.object, "addListener", 1, &arg, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        FVM_KeyboardState state = c->keyboard;
        state.down[FVM_KEY_SPACE] = 1;
        state.last_code = FVM_KEY_SPACE;
        if (kind == 3)
        {
            if (!(fvm_input_set_keyboard_focus(c, clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        rt->config.instruction_limit = 2;
        if (!(deliver(c, kind, selection.object, state) == FVM_ERROR && !t.calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        double x = -1, y = -1;
        if (!(fvm_input_get_pointer(c, &x, &y) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->keyboard.down[FVM_KEY_SPACE] && x == 0 && y == 0))
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
        clean(c);
        /* Three captured recipients and publication consume four units. */
        rt->config.instruction_limit = 4;
        if (!(deliver(c, kind, selection.object, state) == FVM_ERROR && !t.calls))
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
            if (!(fvm_input_get_pointer(c, &x, &y) == FVM_OK && x == 23 && y == 45))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(!c->keyboard_focus))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        clean(c);
        rt->config.instruction_limit = ordinary;
        if (kind == 3)
        {
            if (!(fvm_input_set_keyboard_focus(c, clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        /* Nested handler work shares the event budget; a later callback is not given a fresh one. */
        t.busy = true;
        rt->config.instruction_limit = kind == 3 ? 10 : 9;
        if (!(deliver(c, kind, selection.object, state) == FVM_ERROR && t.calls == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clean(c);
        t.calls = 0;
        t.busy = false;
        rt->config.instruction_limit = ordinary;
        if (kind == 3)
        {
            if (!(fvm_input_set_keyboard_focus(c, clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(deliver(c, kind, selection.object, state) == FVM_OK && t.calls == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clean(c);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
