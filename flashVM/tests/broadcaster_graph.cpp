/* Round-trip a cyclic broadcaster graph, destroy the sender, then collect and dispatch. */
#include "../src/core/runtime.hpp"
#include "flashvm/broadcaster.h"
#include "flashvm/standard.h"
#include "flashvm/serialization.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstdio>
static FVM_Value value(FVM_Object *o) {
    FVM_Value v; fvm_value_undefined(&v); v.type = FVM_VALUE_OBJECT; v.object = o; return v;
}
static FVM_Result receive(FVM_Context *c, FVM_Object *self, unsigned n,
    const FVM_Value *args, FVM_Value *out, void *) {
    assert(n == 1 && args[0].number == 91);
    fvm_value_undefined(out); return fvm_object_set(c, self, "received", args);
}
int main() {
    FVM_Config cfg; FVM_Runtime *r = 0; FVM_Context *a = 0, *b = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &r) == FVM_OK);
    assert(fvm_context_create(r, &a) == FVM_OK && fvm_standard_install(a) == FVM_OK);
    assert(fvm_context_create(r, &b) == FVM_OK && fvm_standard_install(b) == FVM_OK);
    assert(fvm_serialization_register_callback(a, "test.broadcaster.receive", receive) == FVM_OK);
    assert(fvm_serialization_register_callback(b, "test.broadcaster.receive", receive) == FVM_OK);
    FVM_Object *source = 0, *listener = 0;
    assert(fvm_object_create(a, &source) == FVM_OK && fvm_object_create(a, &listener) == FVM_OK);
    assert(fvm_broadcaster_initialize(a, source) == FVM_OK);
    assert(fvm_standard_broadcaster_install(a) == FVM_OK);
    FVM_Value ns; assert(fvm_get_global(a, "AsBroadcaster", &ns) == FVM_OK);
    assert(fvm_object_set(a, source, "namespace", &ns) == FVM_OK);
    assert(fvm_object_define_method_rooted(a, listener, "onValue", receive, 0, 0) == FVM_OK);
    FVM_Value v = value(source), out;
    assert(fvm_object_set(a, listener, "source", &v) == FVM_OK);
    v = value(listener); assert(fvm_object_set(a, source, "listener", &v) == FVM_OK);
    assert(fvm_object_call(a, source, "addListener", 1, &v, &out) == FVM_OK);
    assert(fvm_object_get(a, source, "broadcastMessage", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
    assert(fvm_object_set(a, source, "savedBroadcast", &out) == FVM_OK);
    v = value(source); unsigned char *bytes = 0; size_t size = 0;
    FVM_Result serialized = fvm_value_serialize(a, &v, &bytes, &size);
    if (serialized != FVM_OK) std::fprintf(stderr, "serialize: %d\n", (int)serialized);
    assert(serialized == FVM_OK);
    fvm_context_destroy(a);
    assert(fvm_value_deserialize(b, bytes, size, &v) == FVM_OK); fvm_serialized_free(bytes);
    source = v.object; assert(fvm_gc_pin(b, source) == FVM_OK);
    assert(fvm_gc_collect(b, 0) == FVM_OK);
    assert(fvm_object_get(b, source, "listener", &v) == FVM_OK); listener = v.object;
    assert(fvm_object_get(b, listener, "source", &v) == FVM_OK && v.object == source);
    FVM_Value args[2]; assert(fvm_value_string(args, "onValue") == FVM_OK); fvm_value_number(args + 1, 91);
    assert(fvm_object_call(b, source, "savedBroadcast", 2, args, &out) == FVM_OK);
    assert(fvm_object_call(b, source, "broadcastMessage", 2, args, &out) == FVM_OK);
    assert(fvm_object_get(b, listener, "received", &v) == FVM_OK && v.number == 91);
    assert(fvm_object_get(b, source, "namespace", &ns) == FVM_OK);
    v = value(source);
    assert(fvm_object_call(b, ns.object, "initialize", 1, &v, &out) == FVM_OK);
    assert(fvm_object_get(b, source, "_listeners", &v) == FVM_OK && v.object->resource_result.number == 0);
    assert(!b->gc_frames && !b->gc_native_depth);
    fvm_context_destroy(b); fvm_runtime_destroy(r);
}
