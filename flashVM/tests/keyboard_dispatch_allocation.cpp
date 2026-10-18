/* Sweep native allocation failures: installation preserves globals and keyboard snapshots. */
#include <cstdlib>
#include <new>
static long fail_after = -1;
void *operator new(size_t size)
{
    if (fail_after == 0) throw std::bad_alloc();
    if (fail_after > 0) --fail_after;
    void *memory = std::malloc(size ? size : 1);
    if (!memory) throw std::bad_alloc();
    return memory;
}
void *operator new[](size_t size) { return ::operator new(size); }
void operator delete(void *memory) throw() { std::free(memory); }
void operator delete[](void *memory) throw() { std::free(memory); }
void *operator new(size_t size, const std::nothrow_t &) throw()
{ try { return ::operator new(size); } catch (...) { return 0; } }
void *operator new[](size_t size, const std::nothrow_t &) throw()
{ try { return ::operator new(size); } catch (...) { return 0; } }
void operator delete(void *memory, const std::nothrow_t &) throw() { std::free(memory); }
void operator delete[](void *memory, const std::nothrow_t &) throw() { std::free(memory); }
#include "../src/core/runtime.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstring>

int main()
{
    FVM_Config config; FVM_Runtime *runtime = 0; FVM_Context *context = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    FVM_Object *listener = 0;
    assert(fvm_object_create(context, &listener) == FVM_OK);
    fail_after = 0;
    assert(fvm_input_add_key_listener(context, listener) == FVM_OUT_OF_MEMORY);
    fail_after = -1;
    assert(context->keyboard_listeners.empty());
    assert(fvm_input_add_key_listener(context, listener) == FVM_OK);
    FVM_KeyboardState state;
    std::memset(&state, 0, sizeof(state)); state.struct_size = sizeof(state);
    state.last_code = FVM_KEY_LEFT; state.down[FVM_KEY_LEFT] = 1;
    fail_after = 0;
    assert(fvm_input_add_key_listener(context, listener) == FVM_OK);
    assert(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OUT_OF_MEMORY);
    fail_after = -1;
    assert(!context->keyboard.last_code && !context->keyboard_dispatching && !context->gc_frames);
    assert(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK);
    int removed = -1;
    fail_after = 0;
    assert(fvm_input_remove_key_listener(context, listener, &removed) == FVM_OK && removed);
    assert(fvm_input_dispatch_keyboard(context, &state, 0) == FVM_OK);
    fail_after = -1;
    assert(fvm_gc_collect(context, 0) == FVM_OK);
    FVM_Object *first = 0;
    for (unsigned int i = 0; i < FVM_KEYBOARD_MAX_LISTENERS; ++i)
    {
        assert(fvm_object_create(context, &listener) == FVM_OK);
        if (!i) first = listener;
        assert(fvm_input_add_key_listener(context, listener) == FVM_OK);
    }
    assert(fvm_object_create(context, &listener) == FVM_OK);
    assert(fvm_input_add_key_listener(context, listener) == FVM_ERROR);
    fail_after = 0;
    assert(fvm_input_add_key_listener(context, first) == FVM_OK);
    fail_after = -1;
    assert(context->keyboard_listeners.size() == FVM_KEYBOARD_MAX_LISTENERS);
    fvm_context_destroy(context); fvm_runtime_destroy(runtime);
}
