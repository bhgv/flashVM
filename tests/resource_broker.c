#include "flashvm/capability.h"
#include "flashvm/mutex.h"
#include "flashvm/transfer.h"
#include "flashvm/serialization.h"
#include "flashvm/debug.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static FVM_Result publish(FVM_Context *context, const char *name, FVM_Object *object)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    return fvm_set_global(context, name, &value);
}
int main(void)
{
#ifdef OS_PORTS
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *owner, *client, *second;
    FVM_ResourceBroker *broker;
    FVM_Mutex *mutex;
    FVM_Channel *queue, *network, *file;
    FVM_Object *original, *proxy, *alias;
    FVM_Value value, received, origin;
    FVM_DebugResourceError errors[64];
    char *mutex_uri, *channel_uri;
    size_t count;
    int state;
    unsigned char *bytes;
    size_t length;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &owner) == FVM_OK);
    assert(fvm_context_create(runtime, &client) == FVM_OK);
    assert(fvm_context_create(runtime, &second) == FVM_OK);
    assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
    assert(fvm_broker_port(broker) != 0);
    assert(fvm_broker_import(
               client,
               "fvm://host:1/aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa/bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb/",
               &alias) == FVM_INVALID_ARGUMENT);
    assert(fvm_broker_import(client, "fvm://host:0/a/b/8", &alias) == FVM_INVALID_ARGUMENT);
    assert(fvm_context_set_broker(owner, broker) == FVM_OK);
    assert(fvm_mutex_create(owner, &mutex) == FVM_OK);
    assert(fvm_extensions_wrap_mutex(owner, mutex, &original) == FVM_OK);
    assert(publish(owner, "lock", original) == FVM_OK);
    assert(fvm_broker_export(owner, original, &mutex_uri) == FVM_OK);
    assert(fvm_broker_import(owner, mutex_uri, &alias) == FVM_OK && alias == original);
    assert(fvm_broker_import(client, mutex_uri, &proxy) == FVM_OK);
    assert(publish(client, "lock", proxy) == FVM_OK);
    assert(fvm_broker_import(client, mutex_uri, &alias) == FVM_OK && alias == proxy);
    assert(fvm_object_get(client, proxy, "origin", &origin) == FVM_OK);
    assert(origin.type == FVM_VALUE_STRING && !strcmp(origin.string, mutex_uri));
    assert(fvm_mutex_set(owner, mutex, 0) == FVM_OK);
    /* Calling the nonblocking C API on the imported handle is exercised by
       network-process tests; touch already probes the actual owner here. */
    assert(fvm_object_call(client, proxy, "touch", 0, 0, &value) == FVM_OK && value.boolean);
    assert(fvm_object_call(client, proxy, "reset", 0, 0, &value) == FVM_ERROR);
    assert(fvm_mutex_reset(owner, mutex) == FVM_OK);
    assert(fvm_object_call(client, proxy, "set", 0, 0, &value) == FVM_OK);
    assert(fvm_mutex_touch(mutex, &state) == FVM_OK && state);
    assert(fvm_mutex_set(owner, mutex, 0) == FVM_NOT_FOUND);
    assert(fvm_object_call(client, proxy, "reset", 0, 0, &value) == FVM_OK);
    /* Destruction releases an acquired remote capability using its original
       peer identity, even though the client context is already destroying. */
    assert(fvm_broker_import(second, mutex_uri, &alias) == FVM_OK);
    assert(fvm_object_call(second, alias, "set", 0, 0, &value) == FVM_OK);
    assert(fvm_mutex_touch(mutex, &state) == FVM_OK && state);
    fvm_context_destroy(second);
    assert(fvm_mutex_touch(mutex, &state) == FVM_OK && !state);
    assert(fvm_context_create(runtime, &second) == FVM_OK);
    assert(fvm_channel_create(&queue) == FVM_OK);
    assert(fvm_extensions_wrap_channel(owner, queue, &original) == FVM_OK);
    assert(publish(owner, "queue", original) == FVM_OK);
    assert(fvm_broker_export(owner, original, &channel_uri) == FVM_OK);
    assert(fvm_channel_open_network(client, channel_uri, &network) == FVM_OK);
    fvm_value_number(&value, 123);
    assert(fvm_channel_send_value(client, network, &value, 0) == FVM_OK);
    assert(fvm_channel_receive_value(owner, queue, &received, 0) == FVM_OK &&
           received.number == 123);
    /* Shared capability travelling through a real TCP graph record. */
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = proxy;
    assert(fvm_channel_send_value(client, network, &value, 0) == FVM_OK);
    assert(fvm_channel_receive_value(owner, queue, &received, 0) == FVM_OK);
    assert(fvm_get_global(owner, "lock", &value) == FVM_OK && received.object == value.object);
    assert(fvm_channel_send_value(owner, queue, &value, 0) == FVM_OK);
    assert(fvm_channel_receive_value(client, network, &received, 0) == FVM_OK &&
           received.object == proxy);
    assert(fvm_channel_open_file("broker-capability.bin", "rw", &file) == FVM_OK);
    value.object = proxy;
    assert(fvm_channel_send_value(client, file, &value, 0) == FVM_OK);
    assert(fvm_channel_receive_value(client, file, &received, 0) == FVM_OK &&
           received.object == proxy);
    /* A stored descriptor names a live authority, not an independent mutex. */
    assert(fvm_value_serialize(client, &value, &bytes, &length) == FVM_OK);
    fvm_mutex_close(mutex);
    assert(fvm_debug_set_location(client, "closed-resource.as", 42) == FVM_OK);
    assert(fvm_value_deserialize(client, bytes, length, &received) == FVM_ERROR);
    fvm_serialized_free(bytes);
    assert(fvm_object_call(client, proxy, "touch", 0, 0, &value) == FVM_ERROR);
    assert(fvm_debug_resource_errors(errors, 64, &count, 1) == FVM_OK && count > 0);
    assert(errors[count - 1].line == 42 && !strcmp(errors[count - 1].source, "closed-resource.as"));
    assert(fvm_broker_import(second, mutex_uri, &alias) == FVM_ERROR);
    fvm_channel_close(queue);
    assert(fvm_channel_send_value(client, network, &value, 0) == FVM_ERROR);
    assert(fvm_debug_set_location(owner, "closed-local-channel.as", 77) == FVM_OK);
    fvm_value_number(&value, 1);
    assert(fvm_channel_send_value(owner, queue, &value, 0) == FVM_ERROR);
    assert(fvm_channel_receive_value(owner, queue, &received, 0) == FVM_ERROR);
    assert(fvm_debug_resource_errors(errors, 64, &count, 1) == FVM_OK && count > 0);
    assert(errors[count - 1].line == 77);
    fvm_channel_release(file);
    remove("broker-capability.bin");
    fvm_channel_release(network);
    fvm_channel_release(queue);
    fvm_mutex_release(mutex);
    fvm_broker_stop(broker);
    assert(fvm_broker_import(client, channel_uri, &alias) == FVM_ERROR);
    fvm_broker_release(broker);
    fvm_serialized_free(mutex_uri);
    fvm_serialized_free(channel_uri);
    fvm_context_destroy(second);
    fvm_context_destroy(client);
    fvm_context_destroy(owner);
    fvm_runtime_destroy(runtime);
#else
    FVM_ResourceBroker *broker = 0;
    assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_UNSUPPORTED);
    assert(broker == 0);
#endif
    return 0;
}
