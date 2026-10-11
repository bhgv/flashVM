#include "flashvm/capability.h"
#include "flashvm/mutex.h"
#include "flashvm/worker.h"
#include "flashvm/transfer.h"
#include "flashvm/serialization.h"
#include <assert.h>
#ifdef OS_PORTS
typedef struct Shared
{
    const char *mutex_uri, *channel_uri;
    FVM_Channel *ready;
    unsigned long counter;
} Shared;
static FVM_Result work(FVM_Context *context, const FVM_Value *input, FVM_Value *out, void *userdata)
{
    Shared *shared;
    FVM_Object *lock;
    FVM_Channel *channel;
    FVM_Value value, result;
    FVM_Result r;
    unsigned int i;
    shared = (Shared *)userdata;
    assert(fvm_broker_import(context, shared->mutex_uri, &lock) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = lock;
    assert(fvm_set_global(context, "lock", &value) == FVM_OK);
    if (input->number == 0)
    {
        for (i = 0; i < 100; ++i)
        {
            assert(fvm_object_call(context, lock, "set", 0, 0, &result) == FVM_OK);
            ++shared->counter;
            assert(fvm_object_call(context, lock, "reset", 0, 0, &result) == FVM_OK);
        }
        fvm_value_number(out, 100);
        return FVM_OK;
    }
    channel = 0;
    if (input->number != 1)
        assert(fvm_channel_open_network(context, shared->channel_uri, &channel) == FVM_OK);
    fvm_value_number(&value, 1);
    assert(fvm_channel_send(shared->ready, &value) == FVM_OK);
    if (input->number == 1)
        r = fvm_object_call(context, lock, "set", 0, 0, &result);
    else if (input->number == 2)
        r = fvm_channel_receive_value(context, channel, out, 1);
    else
        r = fvm_channel_send_value(context, channel, &value, 1);
    if (channel)
        fvm_channel_release(channel);
    return r;
}
#endif
int main(void)
{
#ifdef OS_PORTS
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *context;
    FVM_ResourceBroker *broker;
    FVM_Mutex *mutex;
    FVM_Channel *channel;
    FVM_Object *object;
    FVM_Worker *workers[3];
    FVM_Value input, result;
    Shared shared;
    char *mutex_uri, *channel_uri;
    int i, held;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
    assert(fvm_context_set_broker(context, broker) == FVM_OK);
    assert(fvm_mutex_create(context, &mutex) == FVM_OK);
    assert(fvm_extensions_wrap_mutex(context, mutex, &object) == FVM_OK);
    assert(fvm_broker_export(context, object, &mutex_uri) == FVM_OK);
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_channel_set_capacity(channel, 1) == FVM_OK);
    assert(fvm_extensions_wrap_channel(context, channel, &object) == FVM_OK);
    assert(fvm_broker_export(context, object, &channel_uri) == FVM_OK);
    assert(fvm_channel_create(&shared.ready) == FVM_OK);
    shared.counter = 0;
    shared.mutex_uri = mutex_uri;
    shared.channel_uri = channel_uri;
    fvm_value_number(&input, 0);
    for (i = 0; i < 3; ++i)
        assert(fvm_worker_run(&config, work, &input, &shared, &workers[i]) == FVM_OK);
    for (i = 0; i < 3; ++i)
    {
        assert(fvm_worker_join(workers[i], &result) == FVM_OK && result.number == 100);
        fvm_worker_destroy(workers[i]);
    }
    assert(shared.counter == 300);
    assert(fvm_mutex_set(context, mutex, 0) == FVM_OK);
    for (i = 1; i <= 3; ++i)
    {
        if (i == 3)
        {
            fvm_value_number(&input, 9);
            assert(fvm_channel_send(channel, &input) == FVM_OK);
        }
        fvm_value_number(&input, i);
        assert(fvm_worker_run(&config, work, &input, &shared, &workers[0]) == FVM_OK);
        assert(fvm_channel_receive(shared.ready, &result) == FVM_OK);
        assert(fvm_worker_stop(workers[0]) == FVM_OK);
        assert(fvm_worker_join(workers[0], &result) == FVM_ERROR);
        fvm_worker_destroy(workers[0]);
        assert(fvm_mutex_touch(mutex, &held) == FVM_OK && held);
    }
    assert(fvm_mutex_reset(context, mutex) == FVM_OK);
    fvm_channel_release(shared.ready);
    fvm_channel_release(channel);
    fvm_mutex_release(mutex);
    fvm_serialized_free(mutex_uri);
    fvm_serialized_free(channel_uri);
    fvm_broker_stop(broker);
    fvm_broker_release(broker);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
#endif
    return 0;
}
