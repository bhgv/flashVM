/* Independent client heaps share one function authority without sharing VM access. */
#define FVM_REMOTE_FUNCTION_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_remote.cpp"
#include "../ports/thread_port.h"

#ifdef OS_PORTS
struct Client
{
    FVM_Context *context;
    FVM_Object *proxy;
    unsigned index;
};


static void client_calls(void *raw)
{
    Client *client = (Client *)raw;
    for (unsigned i = 0; i < 25; ++i)
    {
        FVM_Value arg, out;
        fvm_value_number(&arg, client->index * 100 + i);
        if (!(fvm_function_invoke(client->context, client->proxy, 0, 1, &arg, &out) == FVM_OK &&
              out.number == 100 + client->index * 100 + i))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}
#endif


int main()
{
#ifdef OS_PORTS
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *source = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_ResourceBroker *broker = 0;
    if (!(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK &&
          fvm_context_set_broker(source, broker) == FVM_OK && fvm_context_function_transfer(source, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Original state;
    state.context = source;
    state.calls = 0;
    state.bias = 100;
    if (!(fvm_object_create(source, &state.capture) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *function = 0;
    if (!(fvm_function_create_native(source, original, &state, state.capture, &function) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    char *uri = 0;
    if (!(fvm_broker_export(source, function, &uri) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Client clients[4];
    FVM_PortThread *threads[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        clients[i].index = i;
        if (!(fvm_context_create(runtime, &clients[i].context) == FVM_OK &&
              fvm_broker_import(clients[i].context, uri, &clients[i].proxy) == FVM_OK &&
              fvm_gc_pin(clients[i].context, clients[i].proxy) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_collect(source, 0) == FVM_OK && fvm_context_function_serve(source, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        threads[i] = fvm_port_thread_start(client_calls, clients + i);
        if (!(threads[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Idempotent enable must not inspect mutable fields of a running original heap. */
    if (!(fvm_context_function_serve(source, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_port_thread_join(threads[i]);
    }
    if (!(fvm_context_function_serve(source, 0) == FVM_OK && state.calls == 100 && !source->gc_frames &&
          !source->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* A stopped authority retains original roots until owner-thread cleanup. */
    fvm_broker_stop(broker);
    if (!(fvm_gc_collect(source, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(source);
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_context_destroy(clients[i].context);
    }
    fvm_serialized_free(uri);
    fvm_broker_release(broker);
    fvm_runtime_destroy(runtime);
#endif
}
