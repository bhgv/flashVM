/* Idle-authority export allocation failures must not publish targets or retain roots. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cstdlib>
#include <new>

static long allowance = -1;


/* Inject a single allocation boundary through every VM new/new[] site. Both
 * allocation and deallocation use malloc/free so ASan can still observe them. */
void *operator new(std::size_t size) throw(std::bad_alloc)
{
    if (allowance == 0)
    {
        throw std::bad_alloc();
    }
    if (allowance > 0)
    {
        --allowance;
    }
    void *p = std::malloc(size ? size : 1);
    if (!p)
    {
        throw std::bad_alloc();
    }
    return p;
}


void operator delete(void *p) throw()
{
    std::free(p);
}


void *operator new[](std::size_t size) throw(std::bad_alloc)
{
    return ::operator new(size);
}


void operator delete[](void *p) throw()
{
    ::operator delete(p);
}


void *operator new(std::size_t size, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new(size);
    }
    catch (...)
    {
        return 0;
    }
}


void operator delete(void *p, const std::nothrow_t &) throw()
{
    ::operator delete(p);
}


void *operator new[](std::size_t size, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new[](size);
    }
    catch (...)
    {
        return 0;
    }
}


void operator delete[](void *p, const std::nothrow_t &) throw()
{
    ::operator delete[](p);
}

#define FVM_REMOTE_FUNCTION_FIXTURE_ONLY
#include "function_remote.cpp"

#ifndef FVM_REMOTE_FUNCTION_ALLOCATION_FIXTURE_ONLY


int main()
{
#ifdef OS_PORTS
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_ResourceBroker *broker = 0;
    if (!(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool succeeded = false;
    unsigned failures = 0;
    /* No clients connect during injection: the broker stays parked in accept. */
    for (long budget = 0; budget < 80 && !succeeded; ++budget)
    {
        FVM_Context *source = 0;
        if (!(fvm_context_create(runtime, &source) == FVM_OK && fvm_context_set_broker(source, broker) == FVM_OK &&
              fvm_context_function_transfer(source, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Original state;
        state.context = source;
        state.calls = 0;
        state.bias = 1;
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
        allowance = budget;
        FVM_Result result = fvm_broker_export(source, function, &uri);
        allowance = -1;
        FVM_BrokerStats stats;
        if (!(fvm_broker_stats(broker, &stats) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (result == FVM_OK)
        {
            succeeded = true;
            if (!(uri && stats.exported_resources == 1 && function->gc_pins == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            char *again = 0;
            if (!(fvm_broker_export(source, function, &again) == FVM_OK && !std::strcmp(uri, again) &&
                  function->gc_pins == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_serialized_free(again);
        }
        else
        {
            ++failures;
            if (!(result == FVM_OUT_OF_MEMORY && !uri && stats.exported_resources == 0 && function->gc_pins == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_serialized_free(uri);
        if (!(fvm_gc_collect(source, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(source);
        if (!(fvm_broker_stats(broker, &stats) == FVM_OK && stats.exported_resources == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(succeeded && failures >= 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_broker_stop(broker);
    fvm_broker_release(broker);
    fvm_runtime_destroy(runtime);
#endif
}

#endif
