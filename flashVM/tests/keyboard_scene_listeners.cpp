/* Observer-only keyboard publication composes with a scene broadcast without duplicate focus delivery. */
#include "clip_input_methods_fixture.hpp"
/* Track independent implicit-focus and explicit observer notification sources. */
struct KeyTrace
{
    unsigned focus; /* Focused MovieClip method entries. */
    unsigned observers; /* Explicit listener entries. */
    FVM_Result result; /* Observer error returned after publication. */
};
/* An implicit focused call is absent during observer-only publication. */
static FVM_Result focused(FVM_Context *c, FVM_Object *, unsigned count,
                          const FVM_Value *, FVM_Value *out, void *raw)
{
    KeyTrace *trace = static_cast<KeyTrace *>(raw);
    assert(!count && c->stage_update_allowed);
    ++trace->focus;
    return fvm_call(c, "updateAfterEvent", 0, 0, out);
}
/* Observer calls publish state first, reject both recursive keyboard routes, and grant no redraw. */
static FVM_Result observer(FVM_Context *c, FVM_Object *, unsigned count,
                           const FVM_Value *, FVM_Value *out, void *raw)
{
    KeyTrace *trace = static_cast<KeyTrace *>(raw);
    assert(!count && c->keyboard_dispatching && !c->stage_update_allowed);
    assert(c->keyboard.down[FVM_KEY_LEFT]);
    assert(fvm_input_dispatch_key_listeners(c, &c->keyboard, 1) == FVM_INVALID_ARGUMENT);
    assert(fvm_input_dispatch_keyboard(c, &c->keyboard, 1) == FVM_INVALID_ARGUMENT);
    ++trace->observers;
    assert(fvm_call(c, "updateAfterEvent", 0, 0, out) == FVM_OK);
    return trace->result;
}
/* Preserve the old focus-first API while permitting namespace-only scene integration. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK);
    FVM_Context *c = clip_context(runtime);
    FVM_Object *root = method_root(c), *listener = 0;
    KeyTrace trace = {0, 0, FVM_OK};
    assert(fvm_object_create(c, &listener) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, root, "onKeyDown", focused, &trace, 0) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, listener, "onKeyDown", observer, &trace, 0) == FVM_OK);
    assert(fvm_input_set_keyboard_focus(c, root) == FVM_OK && fvm_input_add_key_listener(c, listener) == FVM_OK);
    FVM_KeyboardState state = c->keyboard;
    state.down[FVM_KEY_LEFT] = 1;
    assert(fvm_input_dispatch_key_listeners(c, &state, 1) == FVM_OK);
    assert(trace.observers == 1 && !trace.focus && !c->stage_update_pending);
    assert(fvm_movieclip_dispatch_input(c, root, 64) == FVM_OK);
    assert(trace.focus == 1 && c->stage_update_pending);
    int pending = 0;
    assert(fvm_stage_take_update(c, &pending) == FVM_OK && pending);
    assert(fvm_input_dispatch_keyboard(c, &state, 1) == FVM_OK);
    assert(trace.focus == 2 && trace.observers == 2 && c->stage_update_pending);
    assert(fvm_stage_take_update(c, &pending) == FVM_OK && pending);
    trace.result = FVM_UNSUPPORTED;
    state.last_code = FVM_KEY_LEFT;
    assert(fvm_input_dispatch_key_listeners(c, &state, 1) == FVM_UNSUPPORTED);
    assert(c->keyboard.last_code == FVM_KEY_LEFT && trace.focus == 2 && !c->stage_update_pending);
    FVM_Object *focus = 0;
    assert(fvm_input_get_keyboard_focus(c, &focus) == FVM_OK && focus == root);
    assert(fvm_input_dispatch_key_listeners(c, &state, 2) == FVM_INVALID_ARGUMENT);
    assert(fvm_input_dispatch_key_listeners(c, 0, 1) == FVM_INVALID_ARGUMENT);
    assert(!c->keyboard_dispatching);
    input_clean(c);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
}
