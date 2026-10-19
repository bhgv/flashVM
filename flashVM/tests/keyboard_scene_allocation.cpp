/* Observer-only keyboard snapshot OOM preserves unpublished state and restores keyboard reentry. */
#define main scene_key_allocation_fixture_main
#include "swf_sprite_allocation.cpp"
#undef main
#include "clip_input_methods_fixture.hpp"
static unsigned calls; /* Counter for one sequential context's native observer. */
/* Accept the published held snapshot without allocating inside this fixture callback. */
static FVM_Result receive(FVM_Context *c, FVM_Object *, unsigned count,
                          const FVM_Value *, FVM_Value *out, void *)
{
    assert(!count && c->keyboard.down[FVM_KEY_LEFT]);
    ++calls;
    fvm_value_undefined(out);
    return FVM_OK;
}
/* Sweep snapshot/lookup allocations, then prove a fresh event remains usable. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK);
    bool complete = false;
    unsigned failures = 0;
    for (long n = 0; n < 512 && !complete; ++n)
    {
        FVM_Context *c = clip_context(runtime);
        FVM_Object *listener = 0;
        assert(fvm_object_create(c, &listener) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, listener, "onKeyDown", receive, 0, 0) == FVM_OK);
        assert(fvm_input_add_key_listener(c, listener) == FVM_OK);
        FVM_KeyboardState state = c->keyboard;
        state.down[FVM_KEY_LEFT] = 1;
        calls = 0;
        allowance = n;
        FVM_Result result = fvm_input_dispatch_key_listeners(c, &state, 1);
        allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
        assert(!c->keyboard_dispatching);
        input_clean(c);
        if (result == FVM_OK) complete = true;
        else ++failures;
        if (calls) assert(c->keyboard.down[FVM_KEY_LEFT]);
        assert(fvm_input_dispatch_key_listeners(c, &state, 1) == FVM_OK);
        assert(calls >= 1 && fvm_gc_collect(c, 0) == FVM_OK);
        fvm_context_destroy(c);
    }
    assert(complete && failures);
    fvm_runtime_destroy(runtime);
}
