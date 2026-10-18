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
#include "flashvm/movieclip.h"
#include <cassert>
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
        assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
        assert(fvm_context_create(runtime, &context) == FVM_OK && fvm_standard_install(context) == FVM_OK);
        FVM_KeyboardState state;
        std::memset(&state, 0, sizeof(state));
        state.struct_size = sizeof(state);
        state.down[FVM_KEY_LEFT] = 1;
        state.last_code = FVM_KEY_LEFT;
        assert(fvm_input_set_keyboard(context, &state) == FVM_OK);
        fail_after = allocation;
        FVM_Result result = fvm_standard_selection_install(context);
        fail_after = -1;
        if (result == FVM_OK)
            completed = true;
        else
        {
            assert(result == FVM_OUT_OF_MEMORY);
            ++failures;
            assert(context->global->properties.find("Selection") == context->global->properties.end());
            assert(context->gc_frames == 0 && !context->gc_native_depth);
            assert(fvm_gc_collect(context, 0) == FVM_OK);
            assert(fvm_standard_selection_install(context) == FVM_OK);
        }
        /* Snapshot operations remain usable even while every native allocation fails. */
        fail_after = 0;
        assert(fvm_input_set_keyboard(context, &state) == FVM_OK);
        assert(fvm_input_get_keyboard(context, &state) == FVM_OK && state.down[FVM_KEY_LEFT]);
        assert(fvm_input_reset_keyboard(context) == FVM_OK);
        fail_after = -1;
        fvm_context_destroy(context);
        fvm_runtime_destroy(runtime);
    }
    assert(completed && failures > 10);
    FVM_Config config; FVM_Runtime *runtime = 0; FVM_Context *context = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK && fvm_standard_install(context) == FVM_OK);
    assert(fvm_standard_selection_install(context) == FVM_OK);
    FVM_Object *previous = 0, *candidate = 0;
    assert(fvm_movieclip_create(context, "previous", 1, &previous) == FVM_OK);
    assert(fvm_gc_pin(context, previous) == FVM_OK);
    assert(fvm_input_set_keyboard_focus(context, previous) == FVM_OK);
    assert(fvm_movieclip_create(context, "candidate", 1, &candidate) == FVM_OK);
    assert(fvm_gc_pin(context, candidate) == FVM_OK);
    FVM_Value value, selection, out; fvm_value_boolean(&value, 1);
    assert(fvm_object_set(context, candidate, "focusEnabled", &value) == FVM_OK);
    assert(fvm_get_global(context, "Selection", &selection) == FVM_OK);
    fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = candidate;
    unsigned before_publish = 0, after_publish = 0;
    completed = false; failures = 0;
    for (long allocation = 0; allocation < 256 && !completed; ++allocation)
    {
        assert(fvm_input_set_keyboard_focus(context, previous) == FVM_OK);
        fail_after = allocation;
        FVM_Result result = fvm_object_call(context, selection.object, "setFocus", 1, &value, &out);
        fail_after = -1;
        if (result == FVM_OK)
        {
            completed = true;
            assert(out.type == FVM_VALUE_BOOLEAN && out.boolean && context->keyboard_focus == candidate);
        }
        else
        {
            assert(result == FVM_OUT_OF_MEMORY);
            /* Preparation failures preserve old focus; notification failures keep publication. */
            assert(context->keyboard_focus == previous || context->keyboard_focus == candidate);
            if (context->keyboard_focus == previous) ++before_publish;
            else ++after_publish;
            assert(!context->gc_frames && !context->gc_native_depth);
            ++failures;
        }
    }
    assert(completed && failures && before_publish && after_publish);
    fvm_context_destroy(context); fvm_runtime_destroy(runtime);
}
