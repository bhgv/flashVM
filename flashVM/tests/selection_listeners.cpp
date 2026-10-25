/* Selection observers: identity, snapshots, mutation, endpoint arguments, GC and isolation. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "../src/standard/selection_listeners.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cstring>


struct Trace
{
    FVM_Object *selection, *first, *second, *late;
    FVM_Object *old_clip, *new_clip, *seen[16];
    unsigned calls;
    bool mutate;
    FVM_Result failure;
};


static FVM_Value object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = o ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    v.object = o;
    return v;
}


static FVM_Value method(FVM_Context *c, FVM_Object *s, const char *name, FVM_Object *o)
{
    FVM_Value arg = object(o), out;
    if (!(fvm_object_call(c, s, name, 1, &arg, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out;
}


static FVM_Result notify(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    Trace *t = (Trace *)raw;
    if (!(n == 2 && args[0].type == (t->old_clip ? FVM_VALUE_OBJECT : FVM_VALUE_NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(args[1].type == (t->new_clip ? FVM_VALUE_OBJECT : FVM_VALUE_NULL)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (t->old_clip)
    {
        if (!(args[0].object == t->old_clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (t->new_clip)
    {
        if (!(args[1].object == t->new_clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(t->calls < 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t->seen[t->calls++] = self;
    if (t->mutate)
    {
        t->mutate = false;
        if (!(method(c, t->selection, "removeListener", t->first).boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(method(c, t->selection, "removeListener", t->second).boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(method(c, t->selection, "addListener", t->late).type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_undefined(out);
    return t->failure;
}


static FVM_Object *listener(FVM_Context *c, Trace *t, FVM_Object *capture)
{
    FVM_Object *o = 0;
    if (!(fvm_object_create(c, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, o, "onSetFocus", notify, t, capture) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    o->methods["onSetFocus"].collector_entry = true;
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
    FVM_Value invalid_out;
    c->destroying = true;
    if (!(fvm_selection_add_listener(c, 0, 0, 0, &invalid_out, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_selection_remove_listener(c, 0, 0, 0, &invalid_out, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_selection_listener_array(c, 0, 0, 0, &invalid_out, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->destroying = false;
    Trace t;
    std::memset(&t, 0, sizeof(t));
    t.failure = FVM_OK;
    FVM_Value ns, out;
    if (!(fvm_get_global(c, "Selection", &ns) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.selection = ns.object;
    t.late = listener(c, &t, 0);
    t.first = listener(c, &t, t.late);
    t.second = listener(c, &t, 0);
    if (!(method(c, t.selection, "addListener", t.first).type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    method(c, t.selection, "addListener", t.first);
    method(c, t.selection, "addListener", t.second);
    if (!(c->selection_listeners.size() == 2 && c->keyboard_listeners.empty()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    } /* Registrations and their capture are persistent roots. */
    if (!(fvm_object_get(c, t.selection, "_listeners", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(out.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value length;
    if (!(fvm_object_get(c, out.object, "length", &length) == FVM_OK && length.number == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Changing the detached inspection Array must not mutate the registry. */
    FVM_Value zero;
    fvm_value_number(&zero, 0);
    if (!(fvm_object_set(c, out.object, "length", &zero) == FVM_OK && c->selection_listeners.size() == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, t.selection, "_listeners", &zero) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, t.selection, "_listeners", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, out.object, "length", &length) == FVM_OK && length.number == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_selection_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *clip = 0;
    if (!(fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value yes;
    fvm_value_boolean(&yes, 1);
    if (!(fvm_object_set(c, clip, "focusEnabled", &yes) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.new_clip = clip;
    t.mutate = true;
    if (!(method(c, t.selection, "setFocus", clip).boolean && t.calls == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.seen[0] == t.first && t.seen[1] == t.second))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->selection_listeners.size() == 1 && c->selection_listeners[0] == t.late))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Mutation takes effect next transition; same identity and C setters stay silent. */
    if (!(method(c, t.selection, "setFocus", clip).boolean && t.calls == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.old_clip = clip;
    t.new_clip = 0;
    if (!(method(c, t.selection, "setFocus", 0).boolean && t.calls == 3 && t.seen[2] == t.late))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_keyboard_focus(c, clip) == FVM_OK && t.calls == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Namespace replacement does not release context registrations. */
    if (!(fvm_set_global(c, "Selection", &zero) == FVM_OK && fvm_gc_pin(c, t.selection) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.failure = FVM_INVALID_ARGUMENT;
    FVM_Value null_value = object(0);
    if (!(fvm_object_call(c, t.selection, "setFocus", 1, &null_value, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->keyboard_focus && t.calls == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(method(c, t.selection, "removeListener", t.late).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!method(c, t.selection, "removeListener", t.late).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!method(c, t.selection, "removeListener", 0).boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &other) == FVM_OK && other->selection_listeners.empty()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *foreign = 0;
    if (!(fvm_object_create(other, &foreign) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value foreign_value = object(foreign);
    if (!(fvm_object_call(c, t.selection, "addListener", 1, &foreign_value, &out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->selection_listeners.empty()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Bound saturation and duplicate addition without manufacturing thousands of script calls. */
    c->selection_listeners.resize(FVM_SELECTION_MAX_LISTENERS);
    for (size_t i = 0; i < c->selection_listeners.size(); ++i)
    {
        c->selection_listeners[i] = t.late;
    }
    if (!(method(c, t.selection, "addListener", t.late).type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *overflow = 0;
    if (!(fvm_object_create(c, &overflow) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value overflow_value = object(overflow);
    if (!(fvm_object_call(c, t.selection, "addListener", 1, &overflow_value, &out) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->selection_listeners.size() == FVM_SELECTION_MAX_LISTENERS))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->selection_listeners.clear();
    if (!(!c->gc_frames && !c->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_unpin(c, t.selection) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
