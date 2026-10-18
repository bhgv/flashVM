#include "flashvm/exception.h"
#include "flashvm/capability.h"
#include "flashvm/extensions.h"
#include "flashvm/mutex.h"
#include "flashvm/transfer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
#ifdef OS_PORTS
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_ResourceBroker *broker = 0;
    FVM_Mutex *mutex = 0;
    FVM_Channel *file = 0;
    FVM_Object *lock = 0, *reader = 0;
    FVM_Value value, result;
    FVM_ExceptionInfo info, events[64];
    size_t count = 0;
    const char *path = "resource-exception-test.dat";
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_extensions_install(context) == FVM_OK);
    assert(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK);
    assert(fvm_context_set_broker(context, broker) == FVM_OK);
    assert(fvm_mutex_create(context, &mutex) == FVM_OK);
    assert(fvm_extensions_wrap_mutex(context, mutex, &lock) == FVM_OK);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = lock;
    assert(fvm_channel_open_file(path, "w", &file) == FVM_OK);
    assert(fvm_channel_send_value(context, file, &value, 0) == FVM_OK);
    fvm_channel_release(file);
    assert(fvm_mutex_close(mutex) == FVM_OK);
    assert(fvm_channel_open_file(path, "r", &file) == FVM_OK);
    assert(fvm_extensions_wrap_channel(context, file, &reader) == FVM_OK);
    assert(fvm_object_call(context, reader, "receive", 0, 0, &result) == FVM_EXCEPTION);
    assert(fvm_exception_info(context, &info) == FVM_OK);
    assert(info.code == FVM_RESOURCE_CLOSED && !strcmp(info.kind, "mutex"));
    assert(info.resource == fvm_mutex_identity(mutex));
    assert(info.flags & FVM_EXCEPTION_FILE);
    assert(!strcmp(info.path, path));
    assert(info.address[0]);
    assert(fvm_debug_exception_events(events, 64, &count, 1) == FVM_OK && count);
    assert(events[count - 1].code == FVM_RESOURCE_CLOSED);
    assert(!strcmp(events[count - 1].path, path));
    assert(fvm_exception_clear(context) == FVM_OK);
    fvm_channel_release(file);
    fvm_mutex_release(mutex);
    fvm_context_destroy(context);
    fvm_broker_release(broker);
    fvm_runtime_destroy(runtime);
    remove(path);
#endif
    return 0;
}
