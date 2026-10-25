/* Keyboard listeners: rooted mutation snapshots, optional methods, errors, inspection and teardown. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
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
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    return value;
}


static FVM_Result
record(FVM_Context *context, FVM_Object *self, unsigned count, const FVM_Value *, FVM_Value *out, void *raw)
{
    Host *host = (Host *)raw;
    if (!(!count && context->keyboard_dispatching && host->count < 32))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    host->order[host->count++] = self == host->first ? 1 : self == host->second ? 2 : 3;
    if (!(fvm_input_dispatch_keyboard(context, &context->keyboard, 1) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    if (!(!context->destroying))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (host->mutate && self == host->first)
    {
        int removed = 0;
        if (!(fvm_input_remove_key_listener(context, host->second, &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_input_add_key_listener(context, host->added) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(context, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        host->mutate = false;
    }
    if (host->throw_mode == 1)
    {
        throw std::bad_alloc();
    }
    if (host->throw_mode == 2)
    {
        throw 17;
    }
    if (host->exception)
    {
        FVM_Value marker;
        fvm_value_number(&marker, 789);
        return fvm_exception_raise(context, &marker);
    }
    fvm_value_undefined(out);
    return host->fail ? FVM_UNSUPPORTED : FVM_OK;
}


static FVM_Result global_handler(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *raw)
{
    ++((Host *)raw)->globals;
    return FVM_OK;
}


static FVM_Result getter(FVM_Context *context, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    Host *host = (Host *)raw;
    ++host->getters;
    FVM_Object *function = 0;
    FVM_Result result = fvm_function_create_native(context, record, host, 0, &function);
    if (result == FVM_OK)
    {
        *out = object_value(function);
    }
    return result;
}


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0, *other = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK && fvm_context_create(runtime, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(context) == FVM_OK && fvm_standard_keyboard_install(context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Host host;
    std::memset(&host, 0, sizeof(host));
    if (!(fvm_object_create(context, &host.first) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(context, &host.second) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(context, &host.added) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *objects[] = {host.first, host.second, host.added};
    for (unsigned i = 0; i < 3; ++i)
    {
        if (!(fvm_object_define_method_rooted(context, objects[i], "onKeyDown", record, &host, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Trusted fixture entry permits real GC; production ordinary native callbacks retain their barrier. */
        objects[i]->methods["onKeyDown"].collector_entry = true;
    }
    if (!(fvm_input_add_key_listener(context, host.first) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_add_key_listener(context, host.first) == FVM_OK && context->keyboard_listeners.size() == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_add_key_listener(context, host.second) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_add_key_listener(other, host.first) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = 77;
    if (!(fvm_input_remove_key_listener(other, host.first, &removed) == FVM_INVALID_ARGUMENT && removed == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_KeyboardState state;
    std::memset(&state, 0, sizeof(state));
    state.struct_size = sizeof(state);
    state.last_code = FVM_KEY_LEFT;
    state.down[FVM_KEY_LEFT] = 1;
    host.mutate = true;
    if (!(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(host.count == 2 && host.order[0] == 1 && host.order[1] == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!context->keyboard_dispatching && !context->gc_frames))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(host.count == 4 && host.order[2] == 1 && host.order[3] == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value key, snapshot, length;
    if (!(fvm_get_global(context, "Key", &key) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(context, key.object, "_listeners", &snapshot) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(context, snapshot.object, "length", &length) == FVM_OK && length.number == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&length, 0);
    if (!(fvm_object_set(context, snapshot.object, "length", &length) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(context->keyboard_listeners.size() == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_remove_key_listener(context, host.added, &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_remove_key_listener(context, host.added, &removed) == FVM_OK && !removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    host.fail = true;
    state.last_code = FVM_KEY_RIGHT;
    if (!(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(context->keyboard.last_code == FVM_KEY_RIGHT && !context->keyboard_dispatching))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    host.fail = false;
    for (host.throw_mode = 1; host.throw_mode <= 2; ++host.throw_mode)
    {
        if (!(fvm_input_dispatch_keyboard(context, &state, 1) ==
              (host.throw_mode == 1 ? FVM_OUT_OF_MEMORY : FVM_ERROR)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!context->keyboard_dispatching && !context->gc_frames && !context->gc_native_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    host.throw_mode = 0;
    host.exception = true;
    if (!(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_EXCEPTION))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exception_clear(context) == FVM_OK && !context->keyboard_dispatching))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    host.exception = false;
    if (!(fvm_input_remove_key_listener(context, host.first, &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *missing = 0;
    if (!(fvm_object_create(context, &missing) == FVM_OK && fvm_input_add_key_listener(context, missing) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_register_function(context, "onKeyDown", global_handler, &host) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK && !host.globals))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value scalar;
    fvm_value_number(&scalar, 5);
    if (!(fvm_object_set(context, missing, "onKeyDown", &scalar) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK && !host.globals))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *prototype = 0;
    if (!(fvm_object_create(context, &prototype) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_accessor_rooted(context, prototype, "onKeyUp", getter, 0, &host, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set_prototype(context, missing, prototype) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_keyboard(context, &state, 0) == FVM_OK && host.getters == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(host.order[host.count - 1] == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_dispatch_keyboard(context, &state, 2) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    state.down[0] = 1;
    if (!(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!context->keyboard.down[0] && !context->gc_frames && !context->keyboard_dispatching))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(other);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
}
