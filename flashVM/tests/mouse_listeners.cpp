/* Native pointer listeners: snapshots, drag-before-delivery, mutation, errors and collector roots. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "flashvm/mouse.h"
#include "flashvm/input.h"
#include "flashvm/execution.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cstring>
#include <limits>


struct Trace
{
    FVM_Object *first, *second, *late, *clip;
    FVM_Object *seen[16];
    unsigned calls;
    double x, y;
    bool mutate, check_drag;
    FVM_Result result;
};


static FVM_Result receive(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *, FVM_Value *out, void *raw)
{
    Trace *t = (Trace *)raw;
    if (!(!n && t->calls < 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t->seen[t->calls++] = self;
    double x, y;
    if (!(fvm_input_get_pointer(c, &x, &y) == FVM_OK && x == t->x && y == t->y))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_UP, 999, 888) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (t->check_drag)
    {
        FVM_Affine m;
        if (!(fvm_movieclip_get_matrix(c, t->clip, &m) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(m.x0 == t->x && m.y0 == t->y))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (t->mutate)
    {
        t->mutate = false;
        int removed;
        if (!(fvm_input_remove_mouse_listener(c, t->first, &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_input_remove_mouse_listener(c, t->second, &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_input_add_mouse_listener(c, t->late) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c); /* Trusted entry: only the Mouse delivery guard prevents teardown. */
        if (!(c->mouse_dispatching))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_undefined(out);
    return t->result;
}


static FVM_Object *listener(FVM_Context *c, Trace *t, FVM_Object *capture)
{
    FVM_Object *o = 0;
    if (!(fvm_object_create(c, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *names[] = {"onMouseDown", "onMouseMove", "onMouseUp"};
    for (unsigned i = 0; i < 3; ++i)
    {
        if (!(fvm_object_define_method_rooted(c, o, names[i], receive, t, capture) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        o->methods[names[i]].collector_entry = true;
    }
    return o;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_mouse_install(c) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK && fvm_standard_mouse_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_mouse_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Trace t;
    std::memset(&t, 0, sizeof(t));
    t.result = FVM_OK;
    t.late = listener(c, &t, 0);
    t.first = listener(c, &t, t.late);
    t.second = listener(c, &t, 0);
    if (!(fvm_input_add_mouse_listener(c, t.first) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_add_mouse_listener(c, t.first) == FVM_OK && c->mouse_listeners.size() == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_add_mouse_listener(c, t.second) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value ns, snapshot, length, zero;
    if (!(fvm_get_global(c, "Mouse", &ns) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, ns.object, "_listeners", &snapshot) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, snapshot.object, "length", &length) == FVM_OK && length.number == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&zero, 0);
    if (!(fvm_object_set(c, snapshot.object, "length", &zero) == FVM_OK && c->mouse_listeners.size() == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_create(c, "dragged", 1, &t.clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value yes, out;
    fvm_value_boolean(&yes, 1);
    if (!(fvm_object_call(c, t.clip, "startDrag", 1, &yes, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.x = 13;
    t.y = -7;
    t.mutate = t.check_drag = true;
    if (!(fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_DOWN, t.x, t.y) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.calls == 2 && t.seen[0] == t.first && t.seen[1] == t.second))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->mouse_listeners.size() == 1 && c->mouse_listeners[0] == t.late))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.x = 20;
    t.y = 30;
    if (!(fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_MOVE, t.x, t.y) == FVM_OK && t.calls == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.result = FVM_UNSUPPORTED;
    t.x = 21;
    if (!(fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_UP, t.x, t.y) == FVM_UNSUPPORTED && t.calls == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->mouse_dispatching && !c->gc_frames && !c->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    double x, y;
    if (!(fvm_input_get_pointer(c, &x, &y) == FVM_OK && x == 21 && y == 30))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_mouse(c, (FVM_MouseEventType)0, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_MOVE, std::numeric_limits<double>::infinity(), 0) ==
          FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_MOVE, 0, std::numeric_limits<double>::quiet_NaN()) ==
          FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_get_pointer(c, &x, &y) == FVM_OK && x == 21 && y == 30 && t.calls == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = 123;
    if (!(fvm_input_remove_mouse_listener(other, t.late, &removed) == FVM_INVALID_ARGUMENT && removed == 123))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_add_mouse_listener(other, t.late) == FVM_INVALID_ARGUMENT && other->mouse_listeners.empty()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* A noncallable property shadows its native method and is ignored, not a global fallback. */
    if (!(fvm_object_set(c, t.late, "onMouseMove", &zero) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_register_function(c, "onMouseMove", receive, &t) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_MOVE, 1, 2) == FVM_OK && t.calls == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_remove_mouse_listener(c, t.late, &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_remove_mouse_listener(c, t.late, &removed) == FVM_OK && !removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exec_drag_end(c) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
