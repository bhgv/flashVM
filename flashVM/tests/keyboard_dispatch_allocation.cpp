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
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include <cstring>


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *listener = 0;
    if (!(fvm_object_create(context, &listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = 0;
    if (!(fvm_input_add_key_listener(context, listener) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = -1;
    if (!(context->keyboard_listeners.empty()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_add_key_listener(context, listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_KeyboardState state;
    std::memset(&state, 0, sizeof(state));
    state.struct_size = sizeof(state);
    state.last_code = FVM_KEY_LEFT;
    state.down[FVM_KEY_LEFT] = 1;
    fail_after = 0;
    if (!(fvm_input_add_key_listener(context, listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = -1;
    if (!(!context->keyboard.last_code && !context->keyboard_dispatching && !context->gc_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = -1;
    fail_after = 0;
    if (!(fvm_input_remove_key_listener(context, listener, &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_keyboard(context, &state, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = -1;
    if (!(fvm_gc_collect(context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *focus = 0, *selected = 0;
    if (!(fvm_movieclip_create(context, "focused", 1, &focus) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_keyboard_focus(context, focus) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Every native-validation allocation failure preserves selection and caller output. */
    for (long limit = 0; limit < 16; ++limit)
    {
        selected = 0;
        fail_after = limit;
        FVM_Result result = fvm_input_get_keyboard_focus(context, &selected);
        fail_after = -1;
        if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(context->keyboard_focus == focus))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(selected == (result == FVM_OK ? focus : 0)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    state.last_code = FVM_KEY_RIGHT;
    fail_after = 0;
    if (!(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = -1;
    if (!(context->keyboard.last_code == FVM_KEY_LEFT && context->keyboard_focus == focus))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = 0;
    if (!(fvm_input_set_keyboard_focus(context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_get_keyboard_focus(context, &selected) == FVM_OK && !selected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = -1;
    if (!(fvm_gc_collect(context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *first = 0;
    for (unsigned int i = 0; i < FVM_KEYBOARD_MAX_LISTENERS; ++i)
    {
        if (!(fvm_object_create(context, &listener) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!i)
        {
            first = listener;
        }
        if (!(fvm_input_add_key_listener(context, listener) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_object_create(context, &listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_add_key_listener(context, listener) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = 0;
    if (!(fvm_input_add_key_listener(context, first) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_after = -1;
    if (!(context->keyboard_listeners.size() == FVM_KEYBOARD_MAX_LISTENERS))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
}
