/* Sweep native allocation failures: installation preserves globals and keyboard snapshots. */
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <new>

static long fail_after = -1;


void *operator new(size_t size)
{
    if (fail_after == 0)
    {
        throw std::bad_alloc();
    }
    if (fail_after > 0)
    {
        --fail_after;
    }
    void *memory = std::malloc(size ? size : 1);
    if (!memory)
    {
        throw std::bad_alloc();
    }
    return memory;
}

void *operator new[](size_t size)
{
    return ::operator new(size);
}


void operator delete(void *memory) throw()
{
    std::free(memory);
}


void operator delete[](void *memory) throw()
{
    std::free(memory);
}


void *operator new(size_t size, const std::nothrow_t &) throw()
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


void *operator new[](size_t size, const std::nothrow_t &) throw()
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


void operator delete(void *memory, const std::nothrow_t &) throw()
{
    std::free(memory);
}


void operator delete[](void *memory, const std::nothrow_t &) throw()
{
    std::free(memory);
}

#include "../src/core/runtime.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cstring>


int main()
{
    bool completed = false;
    unsigned failures = 0;
    for (long allocation = 0; allocation < 512 && !completed; ++allocation)
    {
        FVM_Config config;
        FVM_Runtime *runtime = 0;
        FVM_Context *context = 0;
        if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_context_create(runtime, &context) == FVM_OK && fvm_standard_install(context) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_KeyboardState state;
        std::memset(&state, 0, sizeof(state));
        state.struct_size = sizeof(state);
        state.down[FVM_KEY_LEFT] = 1;
        state.last_code = FVM_KEY_LEFT;
        if (!(fvm_input_set_keyboard(context, &state) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_after = allocation;
        FVM_Result result = fvm_standard_keyboard_install(context);
        fail_after = -1;
        if (result == FVM_OK)
        {
            completed = true;
        }
        else
        {
            if (!(result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ++failures;
            if (!(context->global->properties.find("Key") == context->global->properties.end()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(context->gc_frames == 0 && !context->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(context, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_standard_keyboard_install(context) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        /* Snapshot operations remain usable even while every native allocation fails. */
        fail_after = 0;
        if (!(fvm_input_set_keyboard(context, &state) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_input_get_keyboard(context, &state) == FVM_OK && state.down[FVM_KEY_LEFT]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_input_reset_keyboard(context) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fail_after = -1;
        fvm_context_destroy(context);
        fvm_runtime_destroy(runtime);
    }
    if (!(completed && failures > 20))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
