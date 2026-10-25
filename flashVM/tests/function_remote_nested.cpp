/* Passed callable interfaces reenter their authority without waiting on its own listener. */
#define FVM_REMOTE_FUNCTION_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_remote.cpp"
#include "../src/capi/function_rpc_constants.h"
#include <new>

#ifdef OS_PORTS
struct Nested
{
    FVM_Context *owner;
    unsigned calls;
    bool relay;
    FVM_Object *back;
};


static FVM_Result nested(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *raw)
{
    Nested *state = (Nested *)raw;
    if (!(c == state->owner && n == 2 && a[0].type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++state->calls;
    if (!state->relay)
    {
        if (a[1].number > 100)
        {
            FVM_Value args[2];
            args[0] = object(state->back);
            fvm_value_number(args + 1, 2);
            FVM_Result result = fvm_function_invoke(c, state->back, 0, 2, args, out);
            if (result == FVM_OK)
            {
                out->number += 200;
            }
            return result;
        }
        if (a[1].number == -2)
        {
            if (!(fvm_value_string_bytes(out, "N\0E", 3) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            return fvm_exception_raise(c, out);
        }
        if (a[1].number == -4)
        {
            return FVM_OUT_OF_MEMORY;
        }
        if (a[1].number == -5)
        {
            throw std::bad_alloc();
        }
        if (a[1].number == -3)
        {
            fvm_resource_fault(c, FVM_RESOURCE_IO, 19, "nested.original", "nested-resource");
            return FVM_ERROR;
        }
        fvm_value_number(out, 200 + a[1].number);
        return FVM_OK;
    }
    if (a[1].number == 0)
    {
        fvm_value_number(out, 11);
        return FVM_OK;
    }
    FVM_Value args[2];
    args[0] = a[0];
    fvm_value_number(args + 1, a[1].number - 1);
    return fvm_function_invoke(c, a[0].object, 0, 2, args, out);
}
#endif


int main()
{
#ifdef OS_PORTS
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *source[2], *client = 0;
    FVM_ResourceBroker *broker = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &client) == FVM_OK &&
          fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Nested states[2];
    FVM_Object *proxy[2];
    char *uri[2];
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_context_create(runtime, source + i) == FVM_OK &&
              fvm_context_set_broker(source[i], broker) == FVM_OK &&
              fvm_context_function_transfer(source[i], 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        states[i].owner = source[i];
        states[i].calls = 0;
        states[i].relay = i == 0;
        states[i].back = 0;
        if (i == 1)
        {
            if (!(fvm_broker_import(source[i], uri[0], &states[i].back) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_Object *fn = 0;
        if (!(fvm_function_create_native(source[i], nested, states + i, states[i].back, &fn) == FVM_OK &&
              fvm_broker_export(source[i], fn, uri + i) == FVM_OK &&
              fvm_broker_import(client, uri[i], proxy + i) == FVM_OK && fvm_gc_pin(client, proxy[i]) == FVM_OK &&
              fvm_gc_collect(source[i], 0) == FVM_OK && fvm_context_function_serve(source[i], 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Value args[2], out;
    args[0] = object(proxy[1]);
    fvm_value_number(args + 1, 2);
    if (!(fvm_function_invoke(client, proxy[0], 0, 2, args, &out) == FVM_OK && out.number == 201))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, 102);
    if (!(fvm_function_invoke(client, proxy[0], 0, 2, args, &out) == FVM_OK && out.number == 211))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    args[0] = object(proxy[0]);
    fvm_value_number(args + 1, 3);
    if (!(fvm_function_invoke(client, proxy[0], 0, 2, args, &out) == FVM_OK && out.number == 11))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, FVM_FUNCTION_RPC_MAX_DEPTH + 2);
    if (!(fvm_function_invoke(client, proxy[0], 0, 2, args, &out) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    args[0] = object(proxy[1]);
    fvm_value_number(args + 1, -1);
    if (!(fvm_function_invoke(client, proxy[0], 0, 2, args, &out) == FVM_EXCEPTION &&
          fvm_exception_get(client, &out) == FVM_OK && fvm_value_string_size(&out) == 3 &&
          !std::memcmp(out.string, "N\0E", 3) && fvm_exception_clear(client) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, -2);
    if (!(fvm_function_invoke(client, proxy[0], 0, 2, args, &out) == FVM_EXCEPTION))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_ExceptionInfo fault;
    if (!(fvm_exception_info(client, &fault) == FVM_OK && fault.code == FVM_RESOURCE_IO &&
          !std::strcmp(fault.operation, "nested.original") && fvm_exception_clear(client) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, -3);
    if (!(fvm_function_invoke(client, proxy[0], 0, 2, args, &out) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, -4);
    if (!(fvm_function_invoke(client, proxy[0], 0, 2, args, &out) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_function_serve(source[1], 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args + 1, 2);
    if (!(fvm_function_invoke(client, proxy[0], 0, 2, args, &out) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_function_serve(source[1], 1) == FVM_OK &&
          fvm_function_invoke(client, proxy[0], 0, 2, args, &out) == FVM_OK && out.number == 201))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_context_function_serve(source[i], 0) == FVM_OK && !source[i]->gc_frames &&
              !source[i]->gc_native_depth && fvm_gc_collect(source[i], 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(source[i]);
        fvm_serialized_free(uri[i]);
    }
    fvm_context_destroy(client);
    fvm_broker_stop(broker);
    fvm_broker_release(broker);
    fvm_runtime_destroy(runtime);
#endif
}
