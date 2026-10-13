#include "flashvm/mutex.h"
#include "flashvm/extensions.h"
#include "flashvm/transfer.h"
#include "flashvm/serialization.h"
#include "flashvm/table.h"
#include "flashvm/debug.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *a, *b, *inspect, *owned_gc;
    FVM_Mutex *m, *owned;
    FVM_Channel *transport, *shared, *file;
    FVM_Object *ma, *mb, *ca, *graph;
    FVM_Value ns, value, result, second, args, list;
    FVM_DebugChannel state;
    unsigned char *data;
    size_t size;
    unsigned long freed;
    int held;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &a) == FVM_OK);
    assert(fvm_context_create(rt, &b) == FVM_OK);
    assert(fvm_context_create(rt, &inspect) == FVM_OK);
    assert(fvm_context_identity(a) != fvm_context_identity(b));
    /* Collection of the last wrapper must not unlock an owned critical section. */
    assert(fvm_context_create(rt, &owned_gc) == FVM_OK);
    assert(fvm_mutex_create(owned_gc, &owned) == FVM_OK);
    assert(fvm_extensions_wrap_mutex(owned_gc, owned, &ma) == FVM_OK);
    assert(fvm_mutex_set(owned_gc, owned, 0) == FVM_OK);
    fvm_mutex_release(owned);
    assert(fvm_gc_collect(owned_gc, &freed) == FVM_OK);
    assert(fvm_mutex_touch(owned, &held) == FVM_OK && held);
    assert(fvm_extensions_wrap_mutex(owned_gc, owned, &ma) == FVM_OK);
    assert(fvm_mutex_reset(owned_gc, owned) == FVM_OK);
    fvm_context_destroy(owned_gc);
    assert(fvm_extensions_install(a) == FVM_OK);
    assert(fvm_extensions_install(a) == FVM_OK);
    assert(fvm_get_global(a, "mutex", &ns) == FVM_OK);
    assert(fvm_object_call(a, ns.object, "new", 0, 0, &result) == FVM_OK);
    assert(fvm_object_get(a, result.object, "origin", &value) == FVM_OK);
    assert(value.type == FVM_VALUE_STRING && !strcmp(value.string, "this"));
    assert(fvm_object_call(a, ns.object, "list", 0, 0, &list) == FVM_OK);
    assert(fvm_object_get(a, list.object, "length", &value) == FVM_OK && value.number == 1);
    assert(fvm_table_get_index(a, list.object, 0, &value) == FVM_OK);
    assert(value.object == result.object);
    assert(fvm_mutex_create(a, &m) == FVM_OK);
    assert(fvm_extensions_wrap_mutex(a, m, &ma) == FVM_OK);
    assert(fvm_extensions_wrap_mutex(b, m, &mb) == FVM_OK);
    assert(fvm_object_get(b, mb, "origin", &value) == FVM_OK);
    assert(value.type == FVM_VALUE_NUMBER && value.number == fvm_context_identity(a));
    assert(fvm_mutex_set(a, m, 0) == FVM_OK);
    assert(fvm_mutex_set(a, m, 0) == FVM_ERROR);
    assert(fvm_mutex_set(b, m, 0) == FVM_NOT_FOUND);
    assert(fvm_mutex_reset(b, m) == FVM_ERROR);
    assert(fvm_object_get(b, mb, "state", &value) == FVM_OK && value.number == 1);
    assert(fvm_object_call(b, mb, "touch", 0, 0, &value) == FVM_OK && value.boolean);
    fvm_value_number(&value, 99);
    assert(fvm_object_set(b, mb, "state", &value) == FVM_OK);
    assert(fvm_object_get(b, mb, "state", &value) == FVM_OK && value.number == 1);
    assert(fvm_mutex_reset(a, m) == FVM_OK);
    assert(fvm_channel_create(&transport) == FVM_OK);
    assert(fvm_channel_create(&shared) == FVM_OK);
    assert(fvm_extensions_wrap_channel(a, shared, &ca) == FVM_OK);
    assert(fvm_object_create(a, &graph) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = ma;
    assert(fvm_object_set(a, graph, "lock", &value) == FVM_OK);
    assert(fvm_object_set(a, graph, "alias", &value) == FVM_OK);
    value.object = ca;
    assert(fvm_object_set(a, graph, "pipe", &value) == FVM_OK);
    value.object = graph;
    assert(fvm_object_set(a, graph, "self", &value) == FVM_OK);
    /* Cyclic retained capability queues are rejected, ordinary graph cycles work. */
    fvm_value_undefined(&second);
    second.type = FVM_VALUE_OBJECT;
    second.object = ca;
    assert(fvm_channel_send_value(a, shared, &second, 0) == FVM_UNSUPPORTED);
    assert(fvm_channel_send_value(a, transport, &second, 0) == FVM_OK);
    assert(fvm_extensions_wrap_channel(a, transport, &second.object) == FVM_OK);
    assert(fvm_channel_send_value(a, shared, &second, 0) == FVM_UNSUPPORTED);
    assert(fvm_channel_receive_value(b, transport, &second, 0) == FVM_OK);
    assert(fvm_channel_send_value(a, transport, &value, 0) == FVM_OK);
    assert(fvm_value_serialize(a, &value, &data, &size) == FVM_UNSUPPORTED);
    assert(fvm_debug_channel_freeze(transport, 1) == FVM_OK);
    assert(fvm_channel_receive_value(b, transport, &result, 0) == FVM_NOT_FOUND);
    assert(fvm_debug_channel_state(transport, &state) == FVM_OK);
    assert(state.pending == 1 && state.frozen);
    assert(fvm_debug_channel_message(inspect, transport, 0, &result) == FVM_OK);
    assert(fvm_object_get(inspect, result.object, "lock", &second) == FVM_OK);
    assert(fvm_object_call(inspect, second.object, "set", 0, 0, &args) == FVM_OK);
    assert(fvm_mutex_touch(m, &held) == FVM_OK && held);
    assert(fvm_object_call(inspect, second.object, "reset", 0, 0, &args) == FVM_OK);
    assert(fvm_object_set(a, graph, "later", &args) == FVM_OK);
    assert(fvm_object_get(inspect, result.object, "later", &args) == FVM_OK);
    assert(args.type == FVM_VALUE_UNDEFINED);
    assert(fvm_debug_channel_state(transport, &state) == FVM_OK && state.pending == 1);
    assert(fvm_debug_channel_freeze(transport, 0) == FVM_OK);
    assert(fvm_channel_receive_value(b, transport, &result, 0) == FVM_OK);
    assert(fvm_object_get(b, result.object, "lock", &value) == FVM_OK && value.object == mb);
    assert(fvm_object_get(b, result.object, "alias", &second) == FVM_OK && second.object == mb);
    assert(fvm_object_get(b, result.object, "self", &second) == FVM_OK);
    assert(second.object == result.object);
    assert(fvm_object_get(b, result.object, "pipe", &second) == FVM_OK);
    fvm_value_number(&args, 42);
    assert(fvm_object_call(b, second.object, "send", 1, &args, &value) == FVM_OK);
    assert(fvm_channel_receive_owned(shared, &value, 0) == FVM_OK && value.number == 42);
    fvm_channel_message_release(&value);
    /* Queued resources outlive their originating interpreter. */
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = ma;
    assert(fvm_channel_send_value(a, transport, &value, 0) == FVM_OK);
    assert(fvm_mutex_set(a, m, 0) == FVM_OK);
    fvm_context_destroy(a);
    assert(fvm_mutex_touch(m, &held) == FVM_OK && !held);
    assert(fvm_channel_receive_value(b, transport, &value, 0) == FVM_OK && value.object == mb);
    assert(fvm_mutex_set(b, m, 0) == FVM_OK);
    assert(fvm_mutex_reset(b, m) == FVM_OK);
    assert(fvm_channel_open_file("mutex-shared-reject.bin", "w", &file) == FVM_OK);
    assert(fvm_channel_send_value(b, file, &value, 0) == FVM_UNSUPPORTED);
    assert(fvm_debug_channel_freeze(file, 1) == FVM_OK);
    fvm_channel_destroy(file);
    assert(fvm_gc_collect(b, &freed) == FVM_OK);
    fvm_context_destroy(inspect);
    fvm_context_destroy(b);
    fvm_mutex_release(m);
    fvm_channel_destroy(shared);
    fvm_channel_destroy(transport);
    fvm_runtime_destroy(rt);
    remove("mutex-shared-reject.bin");
    return 0;
}
