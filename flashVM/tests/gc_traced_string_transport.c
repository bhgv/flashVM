#include <stdlib.h>
#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include "flashvm/table.h"
#include "flashvm/capability.h"
#include "flashvm/extensions.h"
#include "flashvm/serialization.h"
#include "flashvm/worker.h"
#include <stdio.h>
#include <string.h>


static FVM_Result work(FVM_Context *c, const FVM_Value *input, FVM_Value *out, void *raw)
{
    unsigned short text[3] = {'A', 0, 'B'};
    unsigned int i;
    FVM_Value value;
    FVM_GCStringStats stats;
    (void)input;
    (void)raw;
    if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 1000; ++i)
    {
        if (!(fvm_value_string_utf16(c, text, 3, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_set_global(c, "s", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (i % 20 == 19)
        {
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_string_stats(c, &stats) == FVM_OK && stats.buffers == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_get_global(c, "s", &value) == FVM_OK && !memcmp(value.string, "A\0B", 3)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_value_number(out, 1000);
    return FVM_OK;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *source, *dest;
    FVM_Channel *channel, *proxy;
    FVM_Object *object;
    FVM_Value root, value, edge, out;
    FVM_GCStringStats stats;
    FVM_Worker *workers[4];
    unsigned short text[3] = {'A', 0, 'B'};
    unsigned int i;
    int kind;
#ifdef OS_PORTS
    FVM_ResourceBroker *broker;
    FVM_Object *wrapper;
    char *uri;
#endif
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
    if (!(fvm_context_create(rt, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &dest) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_string_mode(source, FVM_GC_STRINGS_TRACED) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_string_mode(dest, FVM_GC_STRINGS_TRACED) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (kind = 0; kind < 3; ++kind)
    {
#ifndef OS_PORTS
        if (kind == 2)
        {
            continue;
        }
#endif
        proxy = 0;
        if (kind == 1)
        {
            remove("gc-traced-transport.bin");
            if (!(fvm_channel_open_file("gc-traced-transport.bin", "rw", &channel) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(fvm_channel_create(&channel) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
#ifdef OS_PORTS
        if (kind == 2)
        {
            if (!(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_context_set_broker(source, broker) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_extensions_wrap_channel(source, channel, &wrapper) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_broker_export(source, wrapper, &uri) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_channel_open_network(dest, uri, &proxy) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
#endif
        if (!(fvm_object_create(source, &object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string_utf16(source, text, 3, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(source, object, "bytes", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_table_set_index(source, object, 0, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&root);
        root.type = FVM_VALUE_OBJECT;
        root.object = object;
        if (!(fvm_object_set(source, object, "self", &root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_send_value(source, channel, &root, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(source, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_string_stats(source, &stats) == FVM_OK && !stats.buffers))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_receive_value(dest, proxy ? proxy : channel, &out, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(dest, out.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(dest, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(dest, out.object, "self", &edge) == FVM_OK && edge.object == out.object))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(dest, out.object, "bytes", &value) == FVM_OK && value.string_size == 3 &&
              !memcmp(value.string, "A\0B", 3)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_table_get_index(dest, out.object, 0, &value) == FVM_OK && !memcmp(value.string, "A\0B", 3)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_unpin(dest, out.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(dest, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_string_stats(dest, &stats) == FVM_OK && !stats.buffers))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (proxy)
        {
            fvm_channel_release(proxy);
        }
        fvm_channel_destroy(channel);
        if (kind == 1)
        {
            if (!(!remove("gc-traced-transport.bin")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
#ifdef OS_PORTS
        if (kind == 2)
        {
            if (!(fvm_context_set_broker(source, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_broker_stop(broker);
            fvm_broker_release(broker);
            fvm_serialized_free(uri);
        }
#endif
    }
    fvm_value_undefined(&value);
    for (i = 0; i < 4; ++i)
    {
        if (!(fvm_worker_run(&cfg, work, &value, 0, &workers[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < 4; ++i)
    {
        if (!(fvm_worker_join(workers[i], &out) == FVM_OK && out.number == 1000))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(workers[i]);
    }
    fvm_context_destroy(dest);
    fvm_context_destroy(source);
    fvm_runtime_destroy(rt);
    return 0;
}
