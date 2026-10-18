/* Selection observers: identity, snapshots, mutation, endpoint arguments, GC and isolation. */
#include "../src/core/runtime.hpp"
#include "../src/standard/selection_listeners.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstring>
struct Trace {
    FVM_Object *selection, *first, *second, *late;
    FVM_Object *old_clip, *new_clip, *seen[16];
    unsigned calls;
    bool mutate;
    FVM_Result failure;
};
static FVM_Value object(FVM_Object *o) {
    FVM_Value v; fvm_value_undefined(&v); v.type = o ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    v.object = o; return v;
}
static FVM_Value method(FVM_Context *c, FVM_Object *s, const char *name, FVM_Object *o) {
    FVM_Value arg = object(o), out;
    assert(fvm_object_call(c, s, name, 1, &arg, &out) == FVM_OK); return out;
}
static FVM_Result notify(FVM_Context *c, FVM_Object *self, unsigned n,
                         const FVM_Value *args, FVM_Value *out, void *raw) {
    Trace *t = (Trace *)raw;
    assert(n == 2 && args[0].type == (t->old_clip ? FVM_VALUE_OBJECT : FVM_VALUE_NULL));
    assert(args[1].type == (t->new_clip ? FVM_VALUE_OBJECT : FVM_VALUE_NULL));
    if (t->old_clip) assert(args[0].object == t->old_clip);
    if (t->new_clip) assert(args[1].object == t->new_clip);
    assert(t->calls < 16); t->seen[t->calls++] = self;
    if (t->mutate) {
        t->mutate = false;
        assert(method(c, t->selection, "removeListener", t->first).boolean);
        assert(method(c, t->selection, "removeListener", t->second).boolean);
        assert(method(c, t->selection, "addListener", t->late).type == FVM_VALUE_UNDEFINED);
        assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    }
    fvm_value_undefined(out); return t->failure;
}
static FVM_Object *listener(FVM_Context *c, Trace *t, FVM_Object *capture) {
    FVM_Object *o = 0; assert(fvm_object_create(c, &o) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, o, "onSetFocus", notify, t, capture) == FVM_OK);
    o->methods["onSetFocus"].collector_entry = true; return o;
}
int main() {
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0, *other = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
    assert(fvm_standard_selection_install(c) == FVM_OK);
    FVM_Value invalid_out;
    c->destroying = true;
    assert(fvm_selection_add_listener(c, 0, 0, 0, &invalid_out, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_selection_remove_listener(c, 0, 0, 0, &invalid_out, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_selection_listener_array(c, 0, 0, 0, &invalid_out, 0) == FVM_INVALID_ARGUMENT);
    c->destroying = false;
    Trace t; std::memset(&t, 0, sizeof(t)); t.failure = FVM_OK;
    FVM_Value ns, out; assert(fvm_get_global(c, "Selection", &ns) == FVM_OK); t.selection = ns.object;
    t.late = listener(c, &t, 0); t.first = listener(c, &t, t.late); t.second = listener(c, &t, 0);
    assert(method(c, t.selection, "addListener", t.first).type == FVM_VALUE_UNDEFINED);
    method(c, t.selection, "addListener", t.first); method(c, t.selection, "addListener", t.second);
    assert(c->selection_listeners.size() == 2 && c->keyboard_listeners.empty());
    assert(fvm_gc_collect(c, 0) == FVM_OK); /* Registrations and their capture are persistent roots. */
    assert(fvm_object_get(c, t.selection, "_listeners", &out) == FVM_OK);
    assert(out.type == FVM_VALUE_OBJECT);
    FVM_Value length;
    assert(fvm_object_get(c, out.object, "length", &length) == FVM_OK && length.number == 2);
    /* Changing the detached inspection Array must not mutate the registry. */
    FVM_Value zero; fvm_value_number(&zero, 0);
    assert(fvm_object_set(c, out.object, "length", &zero) == FVM_OK && c->selection_listeners.size() == 2);
    assert(fvm_object_set(c, t.selection, "_listeners", &zero) == FVM_OK);
    assert(fvm_object_get(c, t.selection, "_listeners", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
    assert(fvm_object_get(c, out.object, "length", &length) == FVM_OK && length.number == 2);
    assert(fvm_standard_selection_install(c) == FVM_OK);
    FVM_Object *clip = 0; assert(fvm_movieclip_create(c, "clip", 1, &clip) == FVM_OK);
    FVM_Value yes; fvm_value_boolean(&yes, 1);
    assert(fvm_object_set(c, clip, "focusEnabled", &yes) == FVM_OK);
    t.new_clip = clip; t.mutate = true;
    assert(method(c, t.selection, "setFocus", clip).boolean && t.calls == 2);
    assert(t.seen[0] == t.first && t.seen[1] == t.second);
    assert(c->selection_listeners.size() == 1 && c->selection_listeners[0] == t.late);
    /* Mutation takes effect next transition; same identity and C setters stay silent. */
    assert(method(c, t.selection, "setFocus", clip).boolean && t.calls == 2);
    t.old_clip = clip; t.new_clip = 0;
    assert(method(c, t.selection, "setFocus", 0).boolean && t.calls == 3 && t.seen[2] == t.late);
    assert(fvm_input_set_keyboard_focus(c, clip) == FVM_OK && t.calls == 3);
    /* Namespace replacement does not release context registrations. */
    assert(fvm_set_global(c, "Selection", &zero) == FVM_OK && fvm_gc_pin(c, t.selection) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    t.failure = FVM_INVALID_ARGUMENT; FVM_Value null_value = object(0);
    assert(fvm_object_call(c, t.selection, "setFocus", 1, &null_value, &out) == FVM_INVALID_ARGUMENT);
    assert(!c->keyboard_focus && t.calls == 4);
    assert(method(c, t.selection, "removeListener", t.late).boolean);
    assert(!method(c, t.selection, "removeListener", t.late).boolean);
    assert(!method(c, t.selection, "removeListener", 0).boolean);
    assert(fvm_context_create(rt, &other) == FVM_OK && other->selection_listeners.empty());
    FVM_Object *foreign = 0; assert(fvm_object_create(other, &foreign) == FVM_OK);
    FVM_Value foreign_value = object(foreign);
    assert(fvm_object_call(c, t.selection, "addListener", 1, &foreign_value, &out) == FVM_INVALID_ARGUMENT);
    assert(c->selection_listeners.empty());
    /* Bound saturation and duplicate addition without manufacturing thousands of script calls. */
    c->selection_listeners.resize(FVM_SELECTION_MAX_LISTENERS);
    for (size_t i = 0; i < c->selection_listeners.size(); ++i) c->selection_listeners[i] = t.late;
    assert(method(c, t.selection, "addListener", t.late).type == FVM_VALUE_UNDEFINED);
    FVM_Object *overflow = 0; assert(fvm_object_create(c, &overflow) == FVM_OK);
    FVM_Value overflow_value = object(overflow);
    assert(fvm_object_call(c, t.selection, "addListener", 1, &overflow_value, &out) == FVM_ERROR);
    assert(c->selection_listeners.size() == FVM_SELECTION_MAX_LISTENERS);
    c->selection_listeners.clear();
    assert(!c->gc_frames && !c->gc_native_depth);
    assert(fvm_gc_unpin(c, t.selection) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
