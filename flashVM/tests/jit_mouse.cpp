/* Real Mouse registration and event handlers under SWF6--8 interpretation/manual/automatic JIT. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/mouse.h"
#include "flashvm/input.h"
#include "flashvm/movieclip.h"
static FVM_Result collect_mouse(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *,
                                FVM_Value *out, void *) {
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    fvm_value_undefined(out); return FVM_OK;
}
static FVM_Object *script(FVM_Context *c, const Program &p, FVM_JitCode **code) {
    FVM_Object *fn = 0;
    assert(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK);
    assert(fvm_gc_pin(c, fn) == FVM_OK);
    assert(fvm_jit_compile(p.bytes.begin(), p.bytes.size(), code) == FVM_OK);
    if (class_mode == 1) assert(fvm_function_set_body(c, fn, fvm_jit_body, *code, 0) == FVM_OK);
    return fn;
}
static FVM_Value invoke(FVM_Context *c, FVM_Object *fn) {
    FVM_Value out; unsigned long entries = c->execution_mode.stats.native_entries;
    assert(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK);
    if (class_mode) assert(c->execution_mode.stats.native_entries > entries);
    return out;
}
int main() {
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 5; version <= 8; ++version)
    for (class_mode = 0; class_mode < 3; ++class_mode) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        if (version < 6) { assert(fvm_standard_mouse_install(c) == FVM_UNSUPPORTED); fvm_context_destroy(c); continue; }
        FVM_Value value; fvm_value_number(&value, 77);
        assert(fvm_set_global(c, "mouse", &value) == FVM_OK);
        assert(fvm_standard_mouse_install(c) == (version == 6 ? FVM_INVALID_ARGUMENT : FVM_OK));
        c->global->properties.erase("mouse"); assert(fvm_standard_mouse_install(c) == FVM_OK);
        assert(fvm_execution_set_mode(c, class_mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK);
        FVM_Object *clip = 0, *observer = 0;
        assert(fvm_movieclip_create(c, "movie", 1, &clip) == FVM_OK);
        value = class_value(clip); assert(fvm_set_global(c, "movie", &value) == FVM_OK);
        assert(fvm_object_create(c, &observer) == FVM_OK);
        value = class_value(observer); assert(fvm_set_global(c, "observer", &value) == FVM_OK);
        assert(fvm_register_function(c, "collectMouse", collect_mouse, 0) == FVM_OK);
        c->functions["collectMouse"].collector_entry = true;
        Program callback;
        callback.number(0); callback.text("collectMouse"); callback.op(0x3d); callback.op(0x17);
        callback.variable("this"); callback.text("pointerX");
        callback.variable("movie"); callback.text("_xmouse"); callback.op(0x4e); callback.op(0x4f);
        callback.number(99); callback.finish();
        FVM_JitCode *handler_code = 0; FVM_Object *handler = script(c, callback, &handler_code);
        value = class_value(handler);
        assert(fvm_object_set(c, observer, "onMouseDown", &value) == FVM_OK);
        assert(fvm_object_set(c, observer, "onMouseMove", &value) == FVM_OK);
        assert(fvm_object_set(c, observer, "onMouseUp", &value) == FVM_OK);
        Program add; add.variable("observer"); add.number(1); add.variable("Mouse");
        add.text("addListener"); add.op(0x52); add.finish();
        FVM_JitCode *add_code = 0; FVM_Object *add_fn = script(c, add, &add_code);
        assert(invoke(c, add_fn).type == FVM_VALUE_UNDEFINED && c->mouse_listeners.size() == 1);
        const FVM_MouseEventType types[] = {FVM_MOUSE_EVENT_DOWN, FVM_MOUSE_EVENT_MOVE, FVM_MOUSE_EVENT_UP};
        for (unsigned i = 0; i < 3; ++i) {
            unsigned long entries = c->execution_mode.stats.native_entries;
            assert(fvm_input_dispatch_mouse(c, types[i], 30 + i, -4) == FVM_OK);
            if (class_mode) assert(c->execution_mode.stats.native_entries > entries);
            assert(fvm_object_get(c, observer, "pointerX", &value) == FVM_OK && value.number == 30 + i);
        }
        Program throwing; throwing.text("pointer failure"); throwing.op(0x2a); throwing.op(0);
        FVM_JitCode *throw_code = 0; FVM_Object *throw_fn = script(c, throwing, &throw_code);
        value = class_value(throw_fn); assert(fvm_object_set(c, observer, "onMouseUp", &value) == FVM_OK);
        assert(fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_UP, 42, 43) == FVM_EXCEPTION);
        assert(c->pointer_world_x == 42 && c->pointer_world_y == 43 && fvm_exception_clear(c) == FVM_OK);
        assert(!c->gc_frames && !c->gc_native_depth && !c->mouse_dispatching && !c->script_depth);
        assert(fvm_function_set_body(c, handler, 0, 0, 0) == FVM_OK && fvm_jit_destroy(handler_code) == FVM_OK);
        assert(fvm_function_set_body(c, add_fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(add_code) == FVM_OK);
        assert(fvm_function_set_body(c, throw_fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(throw_code) == FVM_OK);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
