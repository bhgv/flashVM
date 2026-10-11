#include "flashvm/capability.h"
#include "flashvm/mutex.h"
#include "flashvm/transfer.h"
#include <assert.h>
#include <stdio.h>
/* A file descriptor reconnects to the living mutex; it never copies lock state. */
int main(void)
{
#ifdef OS_PORTS
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *owner, *recipient;
    FVM_ResourceBroker *broker;
    FVM_Mutex *mutex;
    FVM_Channel *file;
    FVM_Object *object;
    FVM_Value value, restored, result;
    int held;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &owner) == FVM_OK);
    assert(fvm_context_create(runtime, &recipient) == FVM_OK);
    assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
    assert(fvm_context_set_broker(owner, broker) == FVM_OK);
    assert(fvm_mutex_create(owner, &mutex) == FVM_OK);
    assert(fvm_extensions_wrap_mutex(owner, mutex, &object) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    assert(fvm_channel_open_file("resource-example.bin", "w", &file) == FVM_OK);
    assert(fvm_channel_send_value(owner, file, &value, 0) == FVM_OK);
    fvm_channel_release(file);
    assert(fvm_channel_open_file("resource-example.bin", "r", &file) == FVM_OK);
    assert(fvm_channel_receive_value(recipient, file, &restored, 0) == FVM_OK);
    assert(fvm_object_call(recipient, restored.object, "set", 0, 0, &result) == FVM_OK);
    assert(fvm_mutex_touch(mutex, &held) == FVM_OK && held);
    assert(fvm_object_call(recipient, restored.object, "reset", 0, 0, &result) == FVM_OK);
    fvm_channel_release(file);
    remove("resource-example.bin");
    fvm_mutex_release(mutex);
    fvm_context_destroy(recipient);
    fvm_broker_stop(broker);
    fvm_broker_release(broker);
    fvm_context_destroy(owner);
    fvm_runtime_destroy(runtime);
    puts("File restored a shared TCP mutex capability");
#endif
    return 0;
}
