/* Real script listener registration and callback bodies agree across interpreter/JIT on SWF6--8. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/keyboard.h"
#include "flashvm/movieclip.h"
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config config; FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version)
    for (class_mode = 0; class_mode < 3; ++class_mode)
    {
        FVM_Context *context = 0;
        assert(fvm_context_create(runtime, &context) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
        swf[3] = (unsigned char)version;
        assert(fvm_load_memory(context, swf, sizeof(swf)) == FVM_OK);
        assert(fvm_standard_install(context) == FVM_OK && fvm_standard_keyboard_install(context) == FVM_OK);
        FVM_Object *listener = 0, *handler = 0;
        assert(fvm_object_create(context, &listener) == FVM_OK);
        FVM_Value value = class_value(listener);
        assert(fvm_set_global(context, "observer", &value) == FVM_OK);
        Program callback;
        callback.variable("this"); callback.text("seen"); callback.number(42); callback.op(0x4f);
        callback.number(1); callback.finish();
        assert(fvm_function_create_avm1(context, callback.bytes.begin(), callback.bytes.size(), 0, 0, 0, &handler) == FVM_OK);
        value = class_value(handler);
        assert(fvm_object_set(context, listener, "onKeyDown", &value) == FVM_OK);
        Program registration;
        registration.variable("observer"); registration.number(1); registration.variable("Key");
        registration.text("addListener"); registration.op(0x52); registration.finish();
        value = class_run(context, registration);
        assert(value.type == FVM_VALUE_UNDEFINED && context->keyboard_listeners.size() == 1);
        FVM_JitCode *code = 0;
        if (class_mode == 1)
        {
            assert(fvm_jit_compile(callback.bytes.begin(), callback.bytes.size(), &code) == FVM_OK);
            assert(fvm_function_set_body(context, handler, fvm_jit_body, code, 0) == FVM_OK);
        }
        assert(fvm_execution_set_mode(context, class_mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK);
        unsigned long native = context->execution_mode.stats.native_entries;
        FVM_KeyboardState state; std::memset(&state, 0, sizeof(state)); state.struct_size = sizeof(state);
        state.last_code = 'A'; state.last_ascii = 'A'; state.down['A'] = 1;
        assert(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK);
        assert(context->execution_mode.stats.native_entries == native + (class_mode != 0));
        assert(fvm_object_get(context, listener, "seen", &value) == FVM_OK && value.number == 42);
        assert(!context->keyboard_dispatching && !context->gc_frames && !context->gc_native_depth);
        assert(fvm_input_dispatch_keyboard(context, &state, 0) == FVM_OK);
        /* The same real script body also receives native MovieClip focus with the clip as this. */
        FVM_Object *clip = 0;
        assert(fvm_movieclip_create(context, "focused", 1, &clip) == FVM_OK);
        assert(fvm_input_set_keyboard_focus(context, clip) == FVM_OK);
        value = class_value(handler);
        assert(fvm_object_set(context, clip, "onKeyDown", &value) == FVM_OK);
        int removed = 0;
        assert(fvm_input_remove_key_listener(context, listener, &removed) == FVM_OK && removed);
        native = context->execution_mode.stats.native_entries;
        assert(fvm_gc_collect(context, 0) == FVM_OK);
        assert(fvm_input_dispatch_keyboard(context, &state, 1) == FVM_OK);
        assert(context->execution_mode.stats.native_entries == native + (class_mode != 0));
        assert(fvm_object_get(context, clip, "seen", &value) == FVM_OK && value.number == 42);
        assert(fvm_input_set_keyboard_focus(context, 0) == FVM_OK);
        assert(fvm_function_set_body(context, handler, 0, 0, 0) == FVM_OK);
        if (code) assert(fvm_jit_destroy(code) == FVM_OK);
        assert(fvm_gc_collect(context, 0) == FVM_OK);
        fvm_context_destroy(context);
    }
    fvm_runtime_destroy(runtime);
}
