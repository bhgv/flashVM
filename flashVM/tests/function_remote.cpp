/* Live native/script functions retain original authority state over one broker listener. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/capability.h"
#include "flashvm/function.h"
#include "flashvm/serialization.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../src/core/capability_internal.hpp"
#include "../src/core/exception_internal.hpp"
#include <cstring>

#ifdef OS_PORTS
struct Original
{
    FVM_Context *context;
    FVM_Object *capture;
    unsigned calls;
    double bias;
};


static FVM_Result original(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *raw)
{
    Original *state = (Original *)raw;
    if (!(c == state->context && state->capture->owner == c))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_context_quiesce(c) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    } /* Running on the broker thread. */
    ++state->calls;
    if (n && a[0].number == -1)
    {
        if (!(fvm_value_string_bytes(out, "E\0X", 3) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return fvm_exception_raise(c, out);
    }
    if (n && a[0].number == -2)
    {
        return FVM_UNSUPPORTED;
    }
    if (n && a[0].number == -3)
    {
        return FVM_ERROR;
    }
    if (n && a[0].number == -4)
    {
        fvm_resource_fault(c, FVM_RESOURCE_IO, 7, "original.test", "original-resource");
        return FVM_ERROR;
    }
    fvm_value_number(out, state->bias + (n ? a[0].number : 0));
    return FVM_OK;
}


static FVM_Value object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = o;
    return v;
}
#endif
#ifndef FVM_REMOTE_FUNCTION_FIXTURE_ONLY


int main()
{
#ifdef OS_PORTS
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *source = 0, *client = 0, *third = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &source) == FVM_OK && fvm_context_create(runtime, &client) == FVM_OK &&
          fvm_context_create(runtime, &third) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_ResourceBroker *broker = 0;
    if (!(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK &&
          fvm_context_set_broker(source, broker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Original states[2];
    FVM_Object *fn[2], *captures[2];
    FVM_Value value, out, arg;
    FVM_Channel *channel = 0;
    if (!(fvm_channel_create(&channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_object_create(source, captures + i) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        states[i].context = source;
        states[i].capture = captures[i];
        states[i].calls = 0;
        states[i].bias = i ? 200 : 100;
        if (!(fvm_function_create_native(source, original, states + i, captures[i], fn + i) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* Unregistered C callbacks cannot be cloned, but can be sent as original-heap capabilities. */
    value = object(fn[0]);
    unsigned char *bytes = 0;
    size_t size = 0;
    if (!(fvm_value_serialize(source, &value, &bytes, &size) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_function_transfer(source, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *proxies[2];
    char *uris[2] = {0, 0};
    for (unsigned i = 0; i < 2; ++i)
    {
        value = object(fn[i]);
        if (!(fvm_broker_export(source, fn[i], uris + i) == FVM_OK &&
              fvm_channel_send_value(source, channel, &value, 0) == FVM_OK &&
              fvm_channel_receive_value(client, channel, &out, 0) == FVM_OK &&
              fvm_gc_pin(client, out.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        proxies[i] = out.object;
        if (!(fvm_function_is_proxy(proxies[i])))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(std::strcmp(uris[0], uris[1]) &&
          !std::strncmp(uris[0], uris[1], (size_t)(std::strrchr(uris[0], '/') - uris[0]) - 32)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(source, 0) == FVM_OK && fvm_context_function_serve(source, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        fvm_value_number(&arg, 7);
        if (!(fvm_function_invoke(client, proxies[i], 0, 1, &arg, &out) == FVM_OK && out.number == (i ? 207 : 107)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_number(&arg, -1);
    if (!(fvm_function_invoke(client, proxies[0], 0, 1, &arg, &out) == FVM_EXCEPTION &&
          fvm_exception_get(client, &out) == FVM_OK && fvm_value_string_size(&out) == 3 &&
          !std::memcmp(out.string, "E\0X", 3) && fvm_exception_clear(client) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&arg, -2);
    if (!(fvm_function_invoke(client, proxies[0], 0, 1, &arg, &out) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&arg, -3);
    if (!(fvm_function_invoke(client, proxies[0], 0, 1, &arg, &out) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&arg, -4);
    if (!(fvm_function_invoke(client, proxies[0], 0, 1, &arg, &out) == FVM_EXCEPTION))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_ExceptionInfo fault;
    if (!(fvm_exception_info(client, &fault) == FVM_OK && fault.code == FVM_RESOURCE_IO &&
          (fault.flags & FVM_EXCEPTION_REMOTE_CONTEXT) && !std::strcmp(fault.operation, "original.test") &&
          fvm_exception_clear(client) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = object(proxies[1]);
    if (!(fvm_channel_send_value(client, channel, &value, 0) == FVM_OK &&
          fvm_channel_receive_value(third, channel, &out, 0) == FVM_OK && fvm_gc_pin(third, out.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *forwarded = out.object;
    fvm_value_number(&arg, 9);
    if (!(fvm_function_invoke(third, forwarded, 0, 1, &arg, &out) == FVM_OK && out.number == 209))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_function_serve(source, 0) == FVM_OK && states[0].calls == 5 && states[1].calls == 2 &&
          !source->gc_frames && !source->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Script lookup must read the original global, not the recipient's globals. */
    const unsigned char code[] = {
        0x96, 13, 0, 0, 'o', 'r', 'i', 'g', 'i', 'n', 'V', 'a', 'l', 'u', 'e', 0, 0x1c, 0x3e, 0};
    fvm_value_number(&value, 17);
    if (!(fvm_set_global(source, "originValue", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 99);
    if (!(fvm_set_global(client, "originValue", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *script = 0;
    if (!(fvm_function_create_avm1(source, code, sizeof(code), 0, 0, 0, &script) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = object(script);
    if (!(fvm_channel_send_value(source, channel, &value, 0) == FVM_OK &&
          fvm_channel_receive_value(client, channel, &out, 0) == FVM_OK && fvm_gc_pin(client, out.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *script_proxy = out.object;
    if (!(fvm_context_function_serve(source, 1) == FVM_OK &&
          fvm_function_invoke(client, script_proxy, 0, 0, 0, &out) == FVM_OK && out.number == 17 &&
          fvm_context_function_serve(source, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Revoke by source teardown: stale proxies fail without dereferencing a freed heap. */
    fvm_context_destroy(source);
    source = 0;
    if (!(fvm_function_invoke(client, proxies[0], 0, 0, 0, &out) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_broker_stop(broker);
    if (!(fvm_function_invoke(third, forwarded, 0, 0, 0, &out) == FVM_EXCEPTION &&
          fvm_exception_clear(third) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_broker_release(broker);
    fvm_serialized_free(uris[0]);
    fvm_serialized_free(uris[1]);
    fvm_channel_release(channel);
    fvm_context_destroy(third);
    fvm_context_destroy(client);
    fvm_runtime_destroy(runtime);
#endif
}
#endif
