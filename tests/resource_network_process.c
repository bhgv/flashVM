#include "flashvm/capability.h"
#include "flashvm/mutex.h"
#include "flashvm/transfer.h"
#include "flashvm/serialization.h"
#include "flashvm/debug.h"
#include "thread_port.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
/* The Python integration driver starts this executable as independent owner and
   receiver processes. With no arguments it stays a normal C89 smoke test. */
int main(int argc, char **argv)
{
#ifdef OS_PORTS
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *context;
    FVM_ResourceBroker *broker;
    FVM_Mutex *mutex;
    FVM_Channel *queue, *network, *file, *reply;
    FVM_Object *object, *lock, *reply_object;
    FVM_Value value, received, result;
    char *lock_uri, *queue_uri;
    unsigned int i;
    int state;
    if (argc < 2)
        return 0;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    if (!strcmp(argv[1], "owner"))
    {
        assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
        assert(fvm_context_set_broker(context, broker) == FVM_OK);
        assert(fvm_mutex_create(context, &mutex) == FVM_OK);
        assert(fvm_extensions_wrap_mutex(context, mutex, &lock) == FVM_OK);
        assert(fvm_broker_export(context, lock, &lock_uri) == FVM_OK);
        assert(fvm_channel_create(&queue) == FVM_OK);
        assert(fvm_channel_set_capacity(queue, 1) == FVM_OK);
        assert(fvm_extensions_wrap_channel(context, queue, &object) == FVM_OK);
        assert(fvm_broker_export(context, object, &queue_uri) == FVM_OK);
        /* Recipient reads this file in another process while the authority lives. */
        assert(fvm_channel_open_file(argv[2], "w", &file) == FVM_OK);
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = lock;
        assert(fvm_channel_send_value(context, file, &value, 0) == FVM_OK);
        value.object = object;
        assert(fvm_channel_send_value(context, file, &value, 0) == FVM_OK);
        assert(fvm_channel_create(&reply) == FVM_OK);
        assert(fvm_extensions_wrap_channel(context, reply, &reply_object) == FVM_OK);
        value.object = reply_object;
        assert(fvm_channel_send_value(context, file, &value, 0) == FVM_OK);
        fvm_channel_release(file);
        assert(fvm_mutex_set(context, mutex, 0) == FVM_OK);
        puts(queue_uri);
        fflush(stdout);
        /* Receive a handshake only after the client has observed contention. */
        assert(fvm_channel_receive_value(context, queue, &received, 1) == FVM_OK);
        assert(received.number == 11);
        assert(fvm_mutex_reset(context, mutex) == FVM_OK);
        assert(fvm_channel_receive_value(context, queue, &received, 1) == FVM_OK);
        assert(received.number == 22);
        assert(fvm_mutex_touch(mutex, &state) == FVM_OK && state);
        assert(fvm_mutex_set(context, mutex, 0) == FVM_NOT_FOUND);
        fvm_value_number(&value, 33);
        assert(fvm_channel_send_value(context, reply, &value, 1) == FVM_OK);
        assert(fvm_channel_receive_value(context, queue, &received, 1) == FVM_OK &&
               received.number == 44);
        for (i = 0; i < 5000; ++i)
        {
            assert(fvm_mutex_touch(mutex, &state) == FVM_OK);
            if (!state)
                break;
            fvm_port_thread_sleep_us(1000);
        }
        assert(!state);
        fvm_mutex_close(mutex);
        fvm_value_number(&value, 55);
        assert(fvm_channel_send_value(context, reply, &value, 1) == FVM_OK);
        assert(fvm_channel_receive_value(context, queue, &received, 1) == FVM_OK &&
               received.number == 66);
        fvm_channel_close(queue);
        fvm_broker_stop(broker);
        fvm_broker_release(broker);
        fvm_mutex_release(mutex);
        fvm_channel_release(queue);
        fvm_channel_release(reply);
        fvm_serialized_free(lock_uri);
        fvm_serialized_free(queue_uri);
    }
    else
    {
        assert(fvm_channel_open_network(context, argv[2], &network) == FVM_OK);
        assert(fvm_channel_open_file(argv[3], "r", &file) == FVM_OK);
        assert(fvm_channel_receive_value(context, file, &received, 0) == FVM_OK);
        lock = received.object;
        assert(fvm_set_global(context, "lock", &received) == FVM_OK);
        assert(fvm_channel_receive_value(context, file, &received, 0) == FVM_OK);
        assert(fvm_set_global(context, "queue", &received) == FVM_OK);
        assert(fvm_channel_receive_value(context, file, &received, 0) == FVM_OK);
        reply_object = received.object;
        assert(fvm_set_global(context, "reply", &received) == FVM_OK);
        assert(fvm_object_call(context, lock, "touch", 0, 0, &result) == FVM_OK && result.boolean);
        assert(fvm_object_call(context, lock, "reset", 0, 0, &result) == FVM_ERROR);
        fvm_value_number(&value, 11);
        assert(fvm_channel_send_value(context, network, &value, 1) == FVM_OK);
        assert(fvm_object_call(context, lock, "set", 0, 0, &result) == FVM_OK);
        fvm_value_number(&value, 22);
        assert(fvm_channel_send_value(context, network, &value, 1) == FVM_OK);
        assert(fvm_object_call(context, reply_object, "receive", 0, 0, &received) == FVM_OK &&
               received.number == 33);
        fvm_value_number(&value, 44);
        assert(fvm_channel_send_value(context, network, &value, 1) == FVM_OK);
        assert(fvm_object_call(context, lock, "reset", 0, 0, &result) == FVM_OK);
        assert(fvm_object_call(context, reply_object, "receive", 0, 0, &received) == FVM_OK &&
               received.number == 55);
        assert(fvm_object_call(context, lock, "touch", 0, 0, &result) == FVM_ERROR);
        fvm_value_number(&value, 66);
        assert(fvm_channel_send_value(context, network, &value, 1) == FVM_OK);
        fvm_channel_release(file);
        fvm_channel_release(network);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
#else
    (void)argc;
    (void)argv;
#endif
    return 0;
}
