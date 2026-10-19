/* Generic broadcasters: mutation snapshots, counted names, GC and transactional reset. */
#include "../src/core/runtime.hpp"
#include "flashvm/broadcaster.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cassert>
#include <stdexcept>

static FVM_Value object_value(FVM_Object *o) {
    FVM_Value v; fvm_value_undefined(&v); v.type = FVM_VALUE_OBJECT; v.object = o; return v;
}
static FVM_Result invoke(FVM_Context *c, FVM_Object *o, const char *name, FVM_Object *arg) {
    FVM_Value v = object_value(arg), out;
    return fvm_object_call(c, o, name, 1, &v, &out);
}
struct State {
    FVM_Object *source, *a, *b, *late, *seen[16];
    unsigned calls; bool mutate, reset; FVM_Result result;
};
static FVM_Result receive(FVM_Context *c, FVM_Object *self, unsigned n,
    const FVM_Value *args, FVM_Value *out, void *data) {
    State *s = static_cast<State *>(data);
    assert(n == 2 && args[0].type == FVM_VALUE_NUMBER && args[0].number == 7);
    assert(args[1].type == FVM_VALUE_STRING && fvm_value_string_size(args + 1) == 3);
    assert(s->calls < 16); s->seen[s->calls++] = self;
    if (s->mutate) {
        s->mutate = false;
        assert(invoke(c, s->source, "removeListener", s->b) == FVM_OK);
        assert(invoke(c, s->source, "addListener", s->late) == FVM_OK);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
    }
    if (s->reset) {
        s->reset = false;
        assert(fvm_broadcaster_initialize(c, s->source) == FVM_OK);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
    }
    fvm_value_undefined(out); return s->result;
}
static FVM_Result unsupported(FVM_Context *, FVM_Object *, unsigned,
    const FVM_Value *, FVM_Value *, void *) { return FVM_UNSUPPORTED; }
static FVM_Result throws(FVM_Context *, FVM_Object *, unsigned,
    const FVM_Value *, FVM_Value *, void *) { throw std::runtime_error("handler"); }
static FVM_Object *make(FVM_Context *c) {
    FVM_Object *o = 0; assert(fvm_object_create(c, &o) == FVM_OK);
    assert(fvm_gc_pin(c, o) == FVM_OK); return o;
}
static void attach(FVM_Context *c, FVM_Object *o, State *s, const char *name, size_t size) {
    assert(fvm_object_define_method_rooted_bytes(c, o, name, size, receive, s, 0) == FVM_OK);
    o->methods[fvm_detail::string(name, size)].collector_entry = true;
}
int main() {
    FVM_Config config; FVM_Runtime *r = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &r) == FVM_OK);
    assert(fvm_context_create(r, &c) == FVM_OK);
    assert(fvm_standard_broadcaster_install(c) == FVM_INVALID_ARGUMENT);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_standard_broadcaster_install(c) == FVM_OK);
    assert(fvm_standard_broadcaster_install(c) == FVM_OK);
    FVM_Value ns, out, v;
    assert(fvm_get_global(c, "AsBroadcaster", &ns) == FVM_OK && ns.type == FVM_VALUE_OBJECT);
    State s; s.source = make(c); s.a = make(c); s.b = make(c); s.late = make(c);
    s.calls = 0; s.mutate = false; s.reset = false; s.result = FVM_OK;
    v = object_value(s.source);
    assert(fvm_object_call(c, ns.object, "initialize", 1, &v, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_UNDEFINED);
    attach(c, s.a, &s, "onTick", 6); attach(c, s.b, &s, "onTick", 6);
    attach(c, s.late, &s, "onTick", 6);
    assert(invoke(c, s.source, "addListener", s.a) == FVM_OK);
    assert(invoke(c, s.source, "addListener", s.b) == FVM_OK);
    assert(invoke(c, s.source, "addListener", s.a) == FVM_OK);
    FVM_Value array; assert(fvm_object_get(c, s.source, "_listeners", &array) == FVM_OK);
    assert(array.object->resource_result.number == 2);
    fvm_value_number(&v, 9);
    assert(fvm_object_set(c, s.source, "_listeners", &v) == FVM_OK);
    assert(fvm_object_get(c, s.source, "_listeners", &v) == FVM_OK && v.object == array.object);
    FVM_Value args[3]; assert(fvm_value_string(args, "onTick") == FVM_OK);
    fvm_value_number(args + 1, 7); assert(fvm_value_string(args + 2, "xyz") == FVM_OK);
    assert(fvm_gc_unpin(c, s.b) == FVM_OK); /* Only list, then dispatch snapshot retains B. */
    s.mutate = true;
    assert(fvm_object_call(c, s.source, "broadcastMessage", 3, args, &out) == FVM_OK);
    assert(s.calls == 2 && s.seen[0] == s.a && s.seen[1] == s.b);
    assert(fvm_object_call(c, s.source, "broadcastMessage", 3, args, &out) == FVM_OK);
    assert(s.calls == 4 && s.seen[2] == s.a && s.seen[3] == s.late);
    unsigned long budget = r->config.instruction_limit; r->config.instruction_limit = 2;
    assert(fvm_object_call(c, s.source, "broadcastMessage", 3, args, &out) == FVM_ERROR);
    assert(s.calls == 4 && !c->avm1_execution_depth);
    r->config.instruction_limit = budget;
    s.result = FVM_UNSUPPORTED;
    assert(fvm_object_call(c, s.source, "broadcastMessage", 3, args, &out) == FVM_UNSUPPORTED);
    assert(s.calls == 5); s.result = FVM_OK;
    const char counted[] = {'o','n',0,'T'};
    attach(c, s.a, &s, counted, sizeof(counted));
    assert(fvm_value_string_bytes(args, counted, sizeof(counted)) == FVM_OK);
    assert(fvm_object_call(c, s.source, "broadcastMessage", 3, args, &out) == FVM_OK);
    assert(s.calls == 6 && s.seen[5] == s.a);
    assert(fvm_value_string(args, "missing") == FVM_OK);
    assert(fvm_object_call(c, s.source, "broadcastMessage", 3, args, &out) == FVM_OK && s.calls == 6);
    assert(fvm_object_define_accessor_rooted(c, s.a, "failure", unsupported, 0, 0, 0) == FVM_OK);
    assert(fvm_value_string(args, "failure") == FVM_OK);
    assert(fvm_object_call(c, s.source, "broadcastMessage", 3, args, &out) == FVM_UNSUPPORTED);
    assert(fvm_object_define_method_rooted(c, s.a, "throwing", throws, 0, 0) == FVM_OK);
    assert(fvm_value_string(args, "throwing") == FVM_OK);
    assert(fvm_object_call(c, s.source, "broadcastMessage", 3, args, &out) == FVM_ERROR);
    assert(!c->gc_frames && !c->gc_native_depth);
    assert(fvm_broadcaster_initialize(c, s.source) == FVM_OK);
    assert(fvm_object_get(c, s.source, "_listeners", &v) == FVM_OK && v.object != array.object);
    assert(v.object->resource_result.number == 0);
    FVM_Object *conflict = make(c); fvm_value_number(&v, 17);
    assert(fvm_object_set(c, conflict, "addListener", &v) == FVM_OK);
    assert(fvm_broadcaster_initialize(c, conflict) == FVM_INVALID_ARGUMENT);
    assert(conflict->accessors.empty() && conflict->methods.empty());
    c->global->methods["AsBroadcaster"].callback = unsupported;
    assert(fvm_standard_broadcaster_install(c) == FVM_INVALID_ARGUMENT);
    c->global->methods.erase(fvm_detail::string("AsBroadcaster"));
    for (unsigned version = 5; version <= 8; ++version) {
        c->swf_loaded = true; c->swf_info.version = version;
        assert(fvm_standard_broadcaster_install(c) == (version < 6 ? FVM_UNSUPPORTED : FVM_OK));
        assert(fvm_broadcaster_initialize(c, s.source) == (version < 6 ? FVM_UNSUPPORTED : FVM_OK));
        assert(fvm_object_call(c, s.source, "broadcastMessage", 3, args, &out) ==
               (version < 6 ? FVM_UNSUPPORTED : FVM_OK));
    }
    c->swf_loaded = false;
    assert(invoke(c, s.source, "addListener", s.a) == FVM_OK);
    assert(invoke(c, s.source, "addListener", s.late) == FVM_OK);
    s.calls = 0; s.reset = true; assert(fvm_value_string(args, "onTick") == FVM_OK);
    assert(fvm_object_call(c, s.source, "broadcastMessage", 3, args, &out) == FVM_OK && s.calls == 2);
    assert(fvm_object_call(c, s.source, "broadcastMessage", 3, args, &out) == FVM_OK && s.calls == 2);
    FVM_Value original; assert(fvm_get_global(c, "Object", &original) == FVM_OK);
    c->global->properties.erase(fvm_detail::string("Object"));
    assert(fvm_object_define_accessor_rooted(c, c->global, "Object", unsupported, 0, 0, original.object) == FVM_OK);
    assert(fvm_broadcaster_initialize(c, s.source) == FVM_UNSUPPORTED);
    assert(fvm_standard_broadcaster_install(c) == FVM_UNSUPPORTED);
    assert(!c->gc_frames && !c->gc_native_depth);
    fvm_context_destroy(c); fvm_runtime_destroy(r);
}
