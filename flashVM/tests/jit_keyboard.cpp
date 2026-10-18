/* Polling executes the same native bridge in interpreted, manual-JIT and automatic-JIT calls. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/keyboard.h"
#include <limits>

int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    for (unsigned version = 5; version <= 8; ++version)
    for (class_mode = 0; class_mode < (fvm_jit_available() ? 3U : 1U); ++class_mode)
    {
        FVM_Context *context = 0;
        assert(fvm_context_create(runtime, &context) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
        swf[3] = (unsigned char)version;
        assert(fvm_load_memory(context, swf, sizeof(swf)) == FVM_OK);
        assert(fvm_standard_install(context) == FVM_OK);
        assert(fvm_standard_keyboard_install(context) == FVM_OK);
        FVM_KeyboardState state;
        std::memset(&state, 0, sizeof(state));
        state.struct_size = sizeof(state);
        state.down[FVM_KEY_LEFT] = state.down[FVM_KEY_RIGHT] = 1;
        state.last_code = FVM_KEY_RIGHT;
        state.last_ascii = 'r';
        assert(fvm_input_set_keyboard(context, &state) == FVM_OK);
        Program polling;
        polling.number(FVM_KEY_LEFT); polling.number(1); polling.variable("Key");
        polling.text("isDown"); polling.op(0x52); polling.finish();
        FVM_Value value = class_run(context, polling);
        assert(value.type == FVM_VALUE_BOOLEAN && value.boolean);
        assert(fvm_input_reset_keyboard(context) == FVM_OK);
        value = class_run(context, polling);
        assert(value.type == FVM_VALUE_BOOLEAN && !value.boolean);
        Program latest;
        latest.number(0); latest.variable("Key"); latest.text("getCode");
        latest.op(0x52); latest.finish();
        value = class_run(context, latest);
        assert(value.type == FVM_VALUE_NUMBER && value.number == 0);
        FVM_Value key, arg;
        assert(fvm_get_global(context, "Key", &key) == FVM_OK);
        fvm_value_number(&arg, std::numeric_limits<double>::quiet_NaN());
        assert(fvm_object_call(context, key.object, "isDown", 1, &arg, &value) == FVM_OK && !value.boolean);
        fvm_value_number(&arg, -1);
        assert(fvm_object_call(context, key.object, "isDown", 1, &arg, &value) == FVM_OK && !value.boolean);
        fvm_value_number(&arg, 256);
        assert(fvm_object_call(context, key.object, "isDown", 1, &arg, &value) == FVM_OK && !value.boolean);
        assert(fvm_gc_collect(context, 0) == FVM_OK);
        fvm_context_destroy(context);
        assert(fvm_context_create(runtime, &context) == FVM_OK);
        assert(fvm_load_memory(context, swf, sizeof(swf)) == FVM_OK);
        assert(fvm_standard_install(context) == FVM_OK);
        fvm_value_number(&arg, 123);
        assert(fvm_set_global(context, "key", &arg) == FVM_OK);
        assert(fvm_standard_keyboard_install(context) == (version <= 6 ? FVM_INVALID_ARGUMENT : FVM_OK));
        assert(fvm_get_global(context, "key", &value) == FVM_OK && value.number == 123);
        fvm_context_destroy(context);
    }
    fvm_runtime_destroy(runtime);
}
