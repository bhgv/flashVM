/* Stopping a broker with persistent mutex peers must not allocate while releasing owners. */
#define FVM_REMOTE_FUNCTION_ALLOCATION_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_remote_export_allocation.cpp"
#include "flashvm/mutex.h"
#include "flashvm/debug.h"


int main()
{
#ifdef OS_PORTS
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *source = 0, *clients[2];
    FVM_ResourceBroker *broker = 0;
    FVM_Mutex *mutex[2];
    FVM_Object *wrappers[2], *proxy[2];
    char *uri[2];
    FVM_Value out;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &source) == FVM_OK &&
          fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK &&
          fvm_context_set_broker(source, broker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_context_create(runtime, clients + i) == FVM_OK && fvm_mutex_create(source, mutex + i) == FVM_OK &&
              fvm_extensions_wrap_mutex(source, mutex[i], wrappers + i) == FVM_OK &&
              fvm_broker_export(source, wrappers[i], uri + i) == FVM_OK &&
              fvm_broker_import(clients[i], uri[i], proxy + i) == FVM_OK &&
              fvm_gc_pin(clients[i], proxy[i]) == FVM_OK &&
              fvm_object_call(clients[i], proxy[i], "set", 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    Original live;
    live.context = source;
    live.calls = 0;
    live.bias = 100;
    FVM_Object *function = 0;
    char *function_uri = 0;
    if (!(fvm_object_create(source, &live.capture) == FVM_OK &&
          fvm_function_create_native(source, original, &live, live.capture, &function) == FVM_OK &&
          fvm_gc_pin(source, function) == FVM_OK && fvm_context_function_transfer(source, 1) == FVM_OK &&
          fvm_broker_export(source, function, &function_uri) == FVM_OK && function->gc_pins == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_BrokerStats stats;
    if (!(fvm_broker_stats(broker, &stats) == FVM_OK && stats.peer_contexts == 2 && stats.exported_resources == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Keep both remote mutexes held: stopping must destroy peer contexts and release ownership. */
    allowance = 0;
    fvm_broker_stop(broker);
    allowance = -1;
    if (!(function->gc_pins == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        int held = 1;
        if (!(fvm_mutex_touch(mutex[i], &held) == FVM_OK && !held && fvm_mutex_set(source, mutex[i], 0) == FVM_OK &&
              fvm_mutex_reset(source, mutex[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(clients[i]);
        fvm_serialized_free(uri[i]);
        fvm_mutex_release(mutex[i]);
    }
    allowance = 0;
    FVM_Result detached = fvm_context_set_broker(source, 0);
    allowance = -1;
    if (!(detached == FVM_OK && function->gc_pins == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(function_uri);
    fvm_context_destroy(source);
    fvm_broker_release(broker);
    fvm_runtime_destroy(runtime);
#endif
}
