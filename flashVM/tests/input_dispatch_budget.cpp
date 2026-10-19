/* Shared native event budgets: preparation rollback, committed partial delivery and nested work. */
#include "../src/core/runtime.hpp"
#include "../src/core/execution_work_scope.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/mouse.h"
#include "flashvm/input.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cassert>
struct Trace { unsigned calls; bool busy; };
static FVM_Value object(FVM_Object *o)
{ FVM_Value v; fvm_value_undefined(&v); v.type = o ? FVM_VALUE_OBJECT : FVM_VALUE_NULL; v.object = o; return v; }
static FVM_Result receive(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    Trace *t = (Trace *)raw; ++t->calls;
    assert(c->avm1_execution_depth && !c->gc_native_depth);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    fvm_value_undefined(out);
    if (t->busy) {
        FVM_ExecutionWorkScope nested(c);
        for (unsigned i = 0; i < 3; ++i) {
            FVM_Result result = nested.step(); if (result != FVM_OK) return result;
        }
    }
    return FVM_OK;
}
static FVM_Result deliver(FVM_Context *c, unsigned kind, FVM_Object *selection, const FVM_KeyboardState &state)
{
    if (!kind) return fvm_input_dispatch_keyboard(c, &state, 1);
    if (kind == 1) return fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_DOWN, 23, 45);
    if (kind == 2) return fvm_input_dispatch_mouse_wheel(c, -2.5, 23, 45);
    FVM_Value arg = object(0), out;
    return fvm_object_call(c, selection, "setFocus", 1, &arg, &out);
}
static void clean(FVM_Context *c)
{ assert(!c->avm1_execution_depth && !c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->mouse_dispatching && !c->keyboard_dispatching); }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const unsigned long ordinary = rt->config.instruction_limit;
    for (unsigned kind = 0; kind < 4; ++kind) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        assert(fvm_standard_selection_install(c) == FVM_OK);
        FVM_Value selection; assert(fvm_get_global(c, "Selection", &selection) == FVM_OK);
        FVM_Object *clip = 0; assert(fvm_movieclip_create(c, "focused", 1, &clip) == FVM_OK);
        assert(fvm_gc_pin(c, clip) == FVM_OK);
        Trace t; t.calls = 0; t.busy = false;
        const char *handler = !kind ? "onKeyDown" : kind == 1 ? "onMouseDown" : kind == 2 ? "onMouseWheel" : "onSetFocus";
        for (unsigned i = 0; i < 3; ++i) {
            FVM_Object *listener = 0; assert(fvm_object_create(c, &listener) == FVM_OK);
            assert(fvm_object_define_method_rooted(c, listener, handler, receive, &t, 0) == FVM_OK);
            listener->methods[handler].collector_entry = true;
            if (!kind) assert(fvm_input_add_key_listener(c, listener) == FVM_OK);
            else if (kind < 3) assert(fvm_input_add_mouse_listener(c, listener) == FVM_OK);
            else { FVM_Value arg = object(listener), out; assert(fvm_object_call(c, selection.object, "addListener", 1, &arg, &out) == FVM_OK); }
        }
        FVM_KeyboardState state = c->keyboard; state.down[FVM_KEY_SPACE] = 1; state.last_code = FVM_KEY_SPACE;
        if (kind == 3) assert(fvm_input_set_keyboard_focus(c, clip) == FVM_OK);
        rt->config.instruction_limit = 2;
        assert(deliver(c, kind, selection.object, state) == FVM_ERROR && !t.calls);
        double x = -1, y = -1;
        assert(fvm_input_get_pointer(c, &x, &y) == FVM_OK);
        assert(!c->keyboard.down[FVM_KEY_SPACE] && x == 0 && y == 0);
        if (kind == 3) assert(c->keyboard_focus == clip);
        clean(c);
        /* Three captured recipients and publication consume four units. */
        rt->config.instruction_limit = 4;
        assert(deliver(c, kind, selection.object, state) == FVM_ERROR && !t.calls);
        if (!kind) assert(c->keyboard.down[FVM_KEY_SPACE]);
        else if (kind < 3) {
            assert(fvm_input_get_pointer(c, &x, &y) == FVM_OK && x == 23 && y == 45);
        }
        else assert(!c->keyboard_focus);
        clean(c);
        rt->config.instruction_limit = ordinary;
        if (kind == 3) assert(fvm_input_set_keyboard_focus(c, clip) == FVM_OK);
        /* Nested handler work shares the event budget; a later callback is not given a fresh one. */
        t.busy = true; rt->config.instruction_limit = kind == 3 ? 10 : 9;
        assert(deliver(c, kind, selection.object, state) == FVM_ERROR && t.calls == 2);
        clean(c);
        t.calls = 0; t.busy = false; rt->config.instruction_limit = ordinary;
        if (kind == 3) assert(fvm_input_set_keyboard_focus(c, clip) == FVM_OK);
        assert(deliver(c, kind, selection.object, state) == FVM_OK && t.calls == 3);
        clean(c); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
