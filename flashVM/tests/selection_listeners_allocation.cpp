/* Reuse the native allocation injector; exercise observer publication and snapshots separately. */
#define main selection_install_allocation_fixture_main
#include "selection_install_allocation.cpp"
#undef main
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
    assert(fvm_standard_selection_install(c) == FVM_OK);
    FVM_Object *listener = 0, *previous = 0, *next = 0;
    assert(fvm_object_create(c, &listener) == FVM_OK && fvm_gc_pin(c, listener) == FVM_OK);
    assert(fvm_movieclip_create(c, "old", 1, &previous) == FVM_OK && fvm_gc_pin(c, previous) == FVM_OK);
    assert(fvm_movieclip_create(c, "next", 1, &next) == FVM_OK && fvm_gc_pin(c, next) == FVM_OK);
    FVM_Value selection, arg, out, yes; fvm_value_boolean(&yes, 1);
    assert(fvm_object_set(c, next, "focusEnabled", &yes) == FVM_OK);
    assert(fvm_get_global(c, "Selection", &selection) == FVM_OK);
    fvm_value_undefined(&arg); arg.type = FVM_VALUE_OBJECT; arg.object = listener;
    bool done = false; unsigned failures = 0;
    for (long allocation = 0; allocation < 256 && !done; ++allocation)
    {
        fail_after = allocation;
        FVM_Result r = fvm_object_call(c, selection.object, "addListener", 1, &arg, &out);
        fail_after = -1;
        if (r == FVM_OK) { done = true; assert(out.type == FVM_VALUE_UNDEFINED); }
        else { assert(r == FVM_OUT_OF_MEMORY && c->selection_listeners.empty()); ++failures; }
        assert(!c->gc_frames && !c->gc_native_depth);
    }
    assert(done && failures && c->selection_listeners.size() == 1);
    assert(fvm_gc_unpin(c, listener) == FVM_OK);
    done = false; failures = 0;
    for (long allocation = 0; allocation < 256 && !done; ++allocation)
    {
        fail_after = allocation;
        FVM_Result r = fvm_object_get(c, selection.object, "_listeners", &out);
        fail_after = -1;
        if (r == FVM_OK) { done = true; assert(out.type == FVM_VALUE_OBJECT); }
        else { assert(r == FVM_OUT_OF_MEMORY); ++failures; }
        assert(c->selection_listeners.size() == 1 && c->selection_listeners[0] == listener);
        assert(!c->gc_frames && !c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    }
    assert(done && failures);
    done = false; unsigned before = 0, after = 0; arg.object = next;
    for (long allocation = 0; allocation < 256 && !done; ++allocation)
    {
        assert(fvm_input_set_keyboard_focus(c, previous) == FVM_OK);
        fail_after = allocation;
        FVM_Result r = fvm_object_call(c, selection.object, "setFocus", 1, &arg, &out);
        fail_after = -1;
        if (r == FVM_OK) { done = true; assert(out.boolean && c->keyboard_focus == next); }
        else
        {
            assert(r == FVM_OUT_OF_MEMORY);
            if (c->keyboard_focus == previous) ++before;
            else { assert(c->keyboard_focus == next); ++after; }
        }
        assert(!c->gc_frames && !c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    }
    assert(done && before && after);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
