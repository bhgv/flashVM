/* Real Selection calls and keyboard script callbacks across SWF6--8 interpreter/JIT modes. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/keyboard.h"
#include "flashvm/movieclip.h"
static FVM_Result collect(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ assert(!c->gc_native_depth); assert(fvm_gc_collect(c, 0) == FVM_OK); fvm_value_boolean(out, 1); return FVM_OK; }
/* Nested script getters may enter JIT too; verify actual native entry without assuming one call. */
static FVM_Value run_selection(FVM_Context *c, const Program &program)
{
    FVM_Object *fn = 0; FVM_JitCode *code = 0; FVM_Value out;
    assert(fvm_function_create_avm1(c, program.bytes.begin(), program.bytes.size(), 0, 0, 0, &fn) == FVM_OK);
    assert(fvm_gc_pin(c, fn) == FVM_OK);
    assert(fvm_jit_compile(program.bytes.begin(), program.bytes.size(), &code) == FVM_OK);
    if (class_mode == 1) assert(fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK);
    assert(fvm_execution_set_mode(c, class_mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK);
    unsigned long entries = c->execution_mode.stats.native_entries;
    assert(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK);
    if (class_mode) assert(c->execution_mode.stats.native_entries > entries);
    else assert(c->execution_mode.stats.native_entries == entries);
    assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !code->active);
    assert(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK);
    assert(fvm_gc_unpin(c, fn) == FVM_OK);
    return out;
}
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; ++version)
    for (class_mode = 0; class_mode < 3; ++class_mode)
    {
        FVM_Context *c = 0;
        assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Value alias; fvm_value_number(&alias, 77);
        assert(fvm_set_global(c, "selection", &alias) == FVM_OK);
        assert(fvm_standard_selection_install(c) == (version == 6 ? FVM_INVALID_ARGUMENT : FVM_OK));
        assert(fvm_get_global(c, "selection", &alias) == FVM_OK && alias.number == 77);
        c->global->properties.erase("selection");
        assert(fvm_standard_selection_install(c) == FVM_OK);
        FVM_Object *clip = 0;
        assert(fvm_movieclip_create(c, "movie", 1, &clip) == FVM_OK);
        FVM_Value value = class_value(clip);
        assert(fvm_set_global(c, "movie", &value) == FVM_OK);
        assert(fvm_register_function(c, "collectFocus", collect, 0) == FVM_OK);
        c->functions["collectFocus"].collector_entry = true;
        Program getter;
        getter.number(0); getter.text("collectFocus"); getter.op(0x3d); getter.finish();
        FVM_Object *fn = 0;
        assert(fvm_function_create_avm1(c, getter.bytes.begin(), getter.bytes.size(), 0, 0, 0, &fn) == FVM_OK);
        FVM_Value args[3]; fvm_value_string(args, "focusEnabled"); args[1] = class_value(fn); fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
        assert(fvm_object_call(c, clip, "addProperty", 3, args, &value) == FVM_OK && value.boolean);
        Program set;
        set.variable("movie"); set.number(1); set.variable("Selection"); set.text("setFocus"); set.op(0x52); set.finish();
        value = run_selection(c, set);
        assert(value.type == FVM_VALUE_BOOLEAN && value.boolean && c->keyboard_focus == clip);
        Program get;
        get.number(0); get.variable("Selection"); get.text("getFocus"); get.op(0x52); get.finish();
        value = run_selection(c, get);
        assert(value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "_level0"));
        /* Real AVM1 focus callbacks collect, preserve receiver and receive the old/new object. */
        Program focus;
        focus.variable("this"); focus.text("previous"); focus.variable("previous"); focus.op(0x4f);
        focus.number(0); focus.text("collectFocus"); focus.op(0x3d); focus.finish();
        const char *parameters[] = {"previous"};
        FVM_Object *focus_handler = 0;
        assert(fvm_function_create_avm1(c, focus.bytes.begin(), focus.bytes.size(), 1, parameters,
                                        0, &focus_handler) == FVM_OK);
        value = class_value(focus_handler);
        assert(fvm_object_set(c, clip, "onSetFocus", &value) == FVM_OK);
        assert(fvm_object_set(c, clip, "onKillFocus", &value) == FVM_OK);
        assert(fvm_input_set_keyboard_focus(c, 0) == FVM_OK);
        value = run_selection(c, set);
        assert(value.boolean && c->keyboard_focus == clip);
        assert(fvm_object_get(c, clip, "previous", &value) == FVM_OK && value.type == FVM_VALUE_NULL);
        Program clear;
        clear.op(0x96); clear.op(1); clear.op(0); clear.op(2); /* Raw Push null. */
        clear.number(1); clear.variable("Selection"); clear.text("setFocus"); clear.op(0x52); clear.finish();
        value = run_selection(c, clear);
        assert(value.boolean && !c->keyboard_focus);
        assert(fvm_object_get(c, clip, "previous", &value) == FVM_OK && value.type == FVM_VALUE_NULL);
        value = run_selection(c, set);
        assert(value.boolean && c->keyboard_focus == clip);
        Program callback;
        callback.variable("this"); callback.text("seen"); callback.number(99); callback.op(0x4f); callback.number(1); callback.finish();
        FVM_Object *handler = 0;
        assert(fvm_function_create_avm1(c, callback.bytes.begin(), callback.bytes.size(), 0, 0, 0, &handler) == FVM_OK);
        value = class_value(handler); assert(fvm_object_set(c, clip, "onKeyDown", &value) == FVM_OK);
        assert(fvm_execution_set_mode(c, class_mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK);
        FVM_KeyboardState state; std::memset(&state, 0, sizeof(state)); state.struct_size = sizeof(state);
        state.last_code = FVM_KEY_LEFT; state.down[FVM_KEY_LEFT] = 1;
        assert(fvm_input_dispatch_keyboard(c, &state, 1) == FVM_OK);
        assert(fvm_object_get(c, clip, "seen", &value) == FVM_OK && value.number == 99);
        assert(!c->gc_frames && !c->gc_native_depth);
        assert(fvm_input_set_keyboard_focus(c, 0) == FVM_OK);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
