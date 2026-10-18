/* Keyboard listeners: rooted mutation snapshots, optional methods, errors, inspection and teardown. */
#include "../src/core/runtime.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include <cassert>
#include <cstring>
#include <new>

struct Host
{
    FVM_Object *first, *second, *added;
    unsigned order[32], count, getters, globals;
    bool mutate, fail, exception;
    unsigned throw_mode;
};
static FVM_Value object_value(FVM_Object *object)
{
    FVM_Value value; fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT; value.object = object; return value;
}
static FVM_Result record(FVM_Context *context, FVM_Object *self, unsigned count,
                          const FVM_Value *, FVM_Value *out, void *raw)
{
    Host *host = (Host *)raw;
    assert(!count && context->keyboard_dispatching && host->count < 32);
    host->order[host->count++] = self == host->first ? 1 : self == host->second ? 2 : 3;
    assert(fvm_input_dispatch_keyboard(context, &context->keyboard, 1) == FVM_INVALID_ARGUMENT);
    fvm_context_destroy(context);
    assert(!context->destroying);
    if (host->mutate && self == host->first)
    {
        int removed = 0;
        assert(fvm_input_remove_key_listener(context, host->second, &removed) == FVM_OK && removed);
        assert(fvm_input_add_key_listener(context, host->added) == FVM_OK);
        assert(fvm_gc_collect(context, 0) == FVM_OK);
        host->mutate = false;
    }
    if (host->throw_mode == 1) throw std::bad_alloc();
    if (host->throw_mode == 2) throw 17;
    if (host->exception)
    {
        FVM_Value marker; fvm_value_number(&marker, 789);
        return fvm_exception_raise(context, &marker);
    }
    fvm_value_undefined(out);
    return host->fail ? FVM_UNSUPPORTED : FVM_OK;
}
static FVM_Result global_handler(FVM_Context *, FVM_Object *, unsigned,
                                  const FVM_Value *, FVM_Value *, void *raw)
{ ++((Host *)raw)->globals; return FVM_OK; }
static FVM_Result getter(FVM_Context *context, FVM_Object *, unsigned,
                          const FVM_Value *, FVM_Value *out, void *raw)
{
    Host *host = (Host *)raw; ++host->getters;
    FVM_Object *function = 0;
    FVM_Result result = fvm_function_create_native(context, record, host, 0, &function);
    if (result == FVM_OK) *out = object_value(function);
    return result;
}
int main()
{
    FVM_Config config; FVM_Runtime *runtime = 0; FVM_Context *context = 0, *other = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK && fvm_context_create(runtime, &other) == FVM_OK);
    assert(fvm_standard_install(context) == FVM_OK && fvm_standard_keyboard_install(context) == FVM_OK);
    Host host; std::memset(&host, 0, sizeof(host));
    assert(fvm_object_create(context, &host.first) == FVM_OK);
    assert(fvm_object_create(context, &host.second) == FVM_OK);
    assert(fvm_object_create(context, &host.added) == FVM_OK);
    FVM_Object *objects[] = {host.first, host.second, host.added};
    for (unsigned i = 0; i < 3; ++i)
    {
        assert(fvm_object_define_method_rooted(context, objects[i], "onKeyDown", record, &host, 0) == FVM_OK);
        /* Trusted fixture entry permits real GC; production ordinary native callbacks retain their barrier. */
        objects[i]->methods["onKeyDown"].collector_entry = true;
    }
    assert(fvm_input_add_key_listener(context, host.first) == FVM_OK);
    assert(fvm_input_add_key_listener(context, host.first) == FVM_OK && context->keyboard_listeners.size() == 1);
    assert(fvm_input_add_key_listener(context, host.second) == FVM_OK);
    assert(fvm_input_add_key_listener(other, host.first) == FVM_INVALID_ARGUMENT);
    int removed = 77;
    assert(fvm_input_remove_key_listener(other, host.first, &removed) == FVM_INVALID_ARGUMENT && removed == 77);
    FVM_KeyboardState state; std::memset(&state, 0, sizeof(state)); state.struct_size = sizeof(state);
    state.last_code = FVM_KEY_LEFT; state.down[FVM_KEY_LEFT] = 1;
    host.mutate = true;
    assert(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK);
    assert(host.count == 2 && host.order[0] == 1 && host.order[1] == 2);
    assert(!context->keyboard_dispatching && !context->gc_frames);
    assert(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK);
    assert(host.count == 4 && host.order[2] == 1 && host.order[3] == 3);
    FVM_Value key, snapshot, length;
    assert(fvm_get_global(context, "Key", &key) == FVM_OK);
    assert(fvm_object_get(context, key.object, "_listeners", &snapshot) == FVM_OK);
    assert(fvm_object_get(context, snapshot.object, "length", &length) == FVM_OK && length.number == 2);
    fvm_value_number(&length, 0);
    assert(fvm_object_set(context, snapshot.object, "length", &length) == FVM_OK);
    assert(context->keyboard_listeners.size() == 2);
    assert(fvm_input_remove_key_listener(context, host.added, &removed) == FVM_OK && removed);
    assert(fvm_input_remove_key_listener(context, host.added, &removed) == FVM_OK && !removed);
    host.fail = true;
    state.last_code = FVM_KEY_RIGHT;
    assert(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_UNSUPPORTED);
    assert(context->keyboard.last_code == FVM_KEY_RIGHT && !context->keyboard_dispatching);
    host.fail = false;
    for (host.throw_mode = 1; host.throw_mode <= 2; ++host.throw_mode)
    {
        assert(fvm_input_dispatch_keyboard(context, &state, 1) == (host.throw_mode == 1 ? FVM_OUT_OF_MEMORY : FVM_ERROR));
        assert(!context->keyboard_dispatching && !context->gc_frames && !context->gc_native_depth);
    }
    host.throw_mode = 0; host.exception = true;
    assert(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_EXCEPTION);
    assert(fvm_exception_clear(context) == FVM_OK && !context->keyboard_dispatching);
    host.exception = false;
    assert(fvm_input_remove_key_listener(context, host.first, &removed) == FVM_OK && removed);
    FVM_Object *missing = 0;
    assert(fvm_object_create(context, &missing) == FVM_OK && fvm_input_add_key_listener(context, missing) == FVM_OK);
    assert(fvm_register_function(context, "onKeyDown", global_handler, &host) == FVM_OK);
    assert(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK && !host.globals);
    FVM_Value scalar; fvm_value_number(&scalar, 5);
    assert(fvm_object_set(context, missing, "onKeyDown", &scalar) == FVM_OK);
    assert(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK && !host.globals);
    FVM_Object *prototype = 0;
    assert(fvm_object_create(context, &prototype) == FVM_OK);
    assert(fvm_object_define_accessor_rooted(context, prototype, "onKeyUp", getter, 0, &host, 0) == FVM_OK);
    assert(fvm_object_set_prototype(context, missing, prototype) == FVM_OK);
    assert(fvm_input_dispatch_keyboard(context, &state, 0) == FVM_OK && host.getters == 1);
    assert(host.order[host.count - 1] == 3);
    assert(fvm_input_dispatch_keyboard(context, &state, 2) == FVM_INVALID_ARGUMENT);
    state.down[0] = 1;
    assert(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_INVALID_ARGUMENT);
    assert(!context->keyboard.down[0] && !context->gc_frames && !context->keyboard_dispatching);
    assert(fvm_gc_collect(context, 0) == FVM_OK);
    fvm_context_destroy(other); fvm_context_destroy(context); fvm_runtime_destroy(runtime);
}
