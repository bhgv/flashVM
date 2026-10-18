/* Strict C89 snapshot validation, isolation, focus reset and polling namespace contracts. */
#include "flashvm/keyboard.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/serialization.h"
#include <assert.h>
#include <string.h>

int main(void)
{
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *context, *other;
    FVM_KeyboardState state, saved, untouched;
    FVM_Value key, argument, result;
    unsigned char *bytes = 0;
    size_t size = 0;
    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_context_create(runtime, &other) == FVM_OK);
    memset(&state, 0, sizeof(state));
    state.struct_size = sizeof(state);
    assert(fvm_input_get_keyboard(context, &state) == FVM_OK);
    assert(!state.last_code && !state.down[FVM_KEY_LEFT]);
    state.down[FVM_KEY_LEFT] = state.down[FVM_KEY_RIGHT] = 1;
    state.last_code = FVM_KEY_RIGHT;
    state.last_ascii = 'A';
    state.caps_lock = 1;
    assert(fvm_input_set_keyboard(context, &state) == FVM_OK);
    saved = state;
    assert(fvm_input_get_keyboard(other, &state) == FVM_OK);
    assert(!state.last_code && !state.down[FVM_KEY_RIGHT]);
    state = saved;
    state.down[255] = 2;
    assert(fvm_input_set_keyboard(context, &state) == FVM_INVALID_ARGUMENT);
    state = saved;
    state.last_code = FVM_KEYBOARD_CODE_COUNT;
    assert(fvm_input_set_keyboard(context, &state) == FVM_INVALID_ARGUMENT);
    state = saved;
    state.last_ascii = FVM_KEYBOARD_ASCII_MAX + 1;
    assert(fvm_input_set_keyboard(context, &state) == FVM_INVALID_ARGUMENT);
    state = saved;
    state.caps_lock = -1;
    assert(fvm_input_set_keyboard(context, &state) == FVM_INVALID_ARGUMENT);
    state = saved;
    state.num_lock = 2;
    assert(fvm_input_set_keyboard(context, &state) == FVM_INVALID_ARGUMENT);
    state = saved;
    state.struct_size = 0;
    untouched = state;
    assert(fvm_input_get_keyboard(context, &state) == FVM_INVALID_ARGUMENT);
    assert(!memcmp(&state, &untouched, sizeof(state)));
    assert(fvm_input_set_keyboard(context, &state) == FVM_INVALID_ARGUMENT);
    assert(fvm_input_set_keyboard(0, &saved) == FVM_INVALID_ARGUMENT);
    assert(fvm_input_get_keyboard(context, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_input_reset_keyboard(0) == FVM_INVALID_ARGUMENT);
    state.struct_size = sizeof(state);
    assert(fvm_input_get_keyboard(context, &state) == FVM_OK);
    assert(!memcmp(&state, &saved, sizeof(state)));
    assert(fvm_standard_keyboard_install(context) == FVM_INVALID_ARGUMENT);
    assert(fvm_standard_install(context) == FVM_OK);
    assert(fvm_standard_keyboard_install(context) == FVM_OK);
    assert(fvm_standard_keyboard_install(context) == FVM_OK);
    assert(fvm_get_global(context, "Key", &key) == FVM_OK);
    fvm_value_number(&argument, FVM_KEY_LEFT);
    assert(fvm_object_call(context, key.object, "isDown", 1, &argument, &result) == FVM_OK);
    assert(result.type == FVM_VALUE_BOOLEAN && result.boolean);
    assert(fvm_object_call(context, key.object, "getCode", 0, 0, &result) == FVM_OK);
    assert(result.type == FVM_VALUE_NUMBER && result.number == FVM_KEY_RIGHT);
    assert(fvm_object_call(context, key.object, "getAscii", 0, 0, &result) == FVM_OK);
    assert(result.number == 'A');
    fvm_value_number(&argument, FVM_KEY_CAPSLOCK);
    assert(fvm_object_call(context, key.object, "isToggled", 1, &argument, &result) == FVM_OK);
    assert(result.type == FVM_VALUE_BOOLEAN && result.boolean);
    assert(fvm_value_serialize(context, &key, &bytes, &size) == FVM_UNSUPPORTED);
    assert(!bytes && !size);
    assert(fvm_input_dispatch_keyboard(context, &saved, 1) == FVM_OK);
    assert(fvm_input_add_key_listener(context, key.object) == FVM_OK);
    {
        int removed = 7;
        assert(fvm_input_remove_key_listener(context, key.object, &removed) == FVM_OK && removed == 1);
    }
    assert(fvm_gc_collect(context, 0) == FVM_OK);
    assert(fvm_input_reset_keyboard(context) == FVM_OK);
    assert(fvm_object_call(context, key.object, "isDown", 1, &argument, &result) == FVM_OK);
    assert(!result.boolean);
    assert(fvm_object_call(context, key.object, "getCode", 0, 0, &result) == FVM_OK);
    assert(!result.number);
    fvm_value_number(&argument, 123);
    assert(fvm_set_global(context, "Key", &argument) == FVM_OK);
    assert(fvm_standard_keyboard_install(context) == FVM_INVALID_ARGUMENT);
    assert(fvm_get_global(context, "Key", &result) == FVM_OK && result.number == 123);
    fvm_context_destroy(other);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
