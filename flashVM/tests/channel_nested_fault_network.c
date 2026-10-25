#include <stdlib.h>
#include "flashvm/capability.h"
#include "flashvm/debug.h"
#include "flashvm/exception.h"
#include "flashvm/extensions.h"
#include "flashvm/mutex.h"
#include "flashvm/serialization.h"
#include "flashvm/transfer.h"
#include <stdio.h>
#include <string.h>

#ifdef OS_PORTS
static FVM_Result
inspect(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    (void)self;
    (void)n;
    (void)args;
    return fvm_debug_channel_message(c, (FVM_Channel *)raw, 0, out);
}
#endif


int main(void)
{
#ifdef OS_PORTS
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *owner, *client;
    FVM_ResourceBroker *broker;
    FVM_Channel *queue, *network, *inner_channel;
    FVM_Mutex *mutex;
    FVM_Object *wrapper, *proxy, *inner;
    FVM_Value value, out;
    FVM_ExceptionInfo fault;
    FVM_DebugChannel state;
    FVM_BrokerStats stats;
    char *uri;
    int kind;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &owner) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &client) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_set_broker(owner, broker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (kind = 0; kind < 3; ++kind)
    {
        if (!(fvm_channel_create(&queue) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_extensions_wrap_channel(owner, queue, &wrapper) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_broker_export(owner, wrapper, &uri) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_open_network(client, uri, &network) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_extensions_wrap_channel(client, network, &proxy) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(client, proxy, "inspect", inspect, network, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (kind == 0)
        {
            if (!(fvm_mutex_create(owner, &mutex) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_extensions_wrap_mutex(owner, mutex, &inner) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (kind == 2)
            {
                remove("nested-inner-resource.bin");
                if (!(fvm_channel_open_file("nested-inner-resource.bin", "rw", &inner_channel) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(fvm_channel_create(&inner_channel) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_extensions_wrap_channel(owner, inner_channel, &inner) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = inner;
        if (!(fvm_channel_send_value(owner, queue, &value, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (kind == 0)
        {
            if (!(fvm_mutex_close(mutex) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            fvm_channel_close(inner_channel);
        }
        if (!(fvm_debug_set_location(client, "nested-capability.as", 29) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(client, proxy, "inspect", 0, 0, &out) == FVM_EXCEPTION))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exception_info(client, &fault) == FVM_OK && fault.code == FVM_RESOURCE_CLOSED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!strcmp(fault.kind, kind == 0 ? "mutex" : "channel") && (fault.flags & FVM_EXCEPTION_REMOTE_CONTEXT)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fault.resource == (kind == 0 ? fvm_mutex_identity(mutex) : fvm_channel_identity(inner_channel))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fault.origin_thread == fvm_context_identity(owner)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!strcmp(fault.operation, "capability.receive")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (kind == 2)
        {
            if (!((fault.flags & FVM_EXCEPTION_FILE) && !strcmp(fault.path, "nested-inner-resource.bin")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fault.line == 29 && !strcmp(fault.source, "nested-capability.as") && !strcmp(fault.address, uri)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_debug_channel_state(network, &state) == FVM_OK && state.pending == 1 && !state.closed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exception_clear(client) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_broker_stats(broker, &stats) == FVM_OK && !stats.peer_contexts && !stats.adapter_contexts))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(client, proxy, "receive", 0, 0, &out) == FVM_EXCEPTION))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exception_info(client, &fault) == FVM_OK && fault.code == FVM_RESOURCE_CLOSED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!strcmp(fault.kind, kind == 0 ? "mutex" : "channel") && (fault.flags & FVM_EXCEPTION_REMOTE_CONTEXT)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fault.resource == (kind == 0 ? fvm_mutex_identity(mutex) : fvm_channel_identity(inner_channel))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fault.origin_thread == fvm_context_identity(owner)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!strcmp(fault.operation, "capability.receive")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (kind == 2)
        {
            if (!((fault.flags & FVM_EXCEPTION_FILE) && !strcmp(fault.path, "nested-inner-resource.bin")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fault.line == 29 && !strcmp(fault.source, "nested-capability.as") &&
              (fault.flags & FVM_EXCEPTION_NETWORK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_debug_channel_state(network, &state) == FVM_OK && !state.pending && !state.closed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exception_clear(client) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_broker_stats(broker, &stats) == FVM_OK && !stats.peer_contexts && !stats.adapter_contexts))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 19);
        if (!(fvm_channel_send(queue, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_receive_value(client, network, &out, 0) == FVM_OK && out.number == 19))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_release(network);
        fvm_channel_release(queue);
        if (kind == 0)
        {
            fvm_mutex_release(mutex);
        }
        else
        {
            fvm_channel_release(inner_channel);
        }
        fvm_serialized_free(uri);
        if (kind == 2)
        {
            if (!(!remove("nested-inner-resource.bin")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_broker_stop(broker);
    fvm_broker_release(broker);
    fvm_context_destroy(client);
    fvm_context_destroy(owner);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
