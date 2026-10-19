/* Reuse the controlled native allocator to check Mouse installation, registration and dispatch. */
#define main selection_install_allocation_fixture_main
#include "selection_install_allocation.cpp"
#undef main
#include "flashvm/mouse.h"
int main() {
    bool done = false; unsigned failed = 0;
    for (long allocation = 0; allocation < 512 && !done; ++allocation) {
        FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
        assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
        assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        fail_after = allocation; FVM_Result r = fvm_standard_mouse_install(c); fail_after = -1;
        if (r == FVM_OK) done = true;
        else {
            assert(r == FVM_OUT_OF_MEMORY); ++failed;
            assert(c->global->properties.find("Mouse") == c->global->properties.end());
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->mouse_dispatching);
        fvm_context_destroy(c); fvm_runtime_destroy(rt);
    }
    assert(done && failed > 10);
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0; FVM_Object *listener = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK && fvm_object_create(c, &listener) == FVM_OK);
    fail_after = 0;
    assert(fvm_input_add_mouse_listener(c, listener) == FVM_OUT_OF_MEMORY && c->mouse_listeners.empty());
    fail_after = -1;
    assert(fvm_input_add_mouse_listener(c, listener) == FVM_OK);
    done = false; unsigned before = 0, after = 0;
    for (long allocation = 0; allocation < 128 && !done; ++allocation) {
        assert(fvm_input_set_pointer(c, 3, 4) == FVM_OK);
        fail_after = allocation;
        FVM_Result r = fvm_input_dispatch_mouse(c, FVM_MOUSE_EVENT_MOVE, 8, 9);
        fail_after = -1;
        double x, y; assert(fvm_input_get_pointer(c, &x, &y) == FVM_OK);
        if (r == FVM_OK) { done = true; assert(x == 8 && y == 9); }
        else {
            assert(r == FVM_OUT_OF_MEMORY);
            if (x == 3) { assert(y == 4); ++before; }
            else { assert(x == 8 && y == 9); ++after; }
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->mouse_dispatching);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
    }
    assert(done && before && after);
    done = false; before = after = 0;
    for (long allocation = 0; allocation < 128 && !done; ++allocation) {
        assert(fvm_input_set_pointer(c, 3, 4) == FVM_OK);
        fail_after = allocation;
        FVM_Result status = fvm_input_dispatch_mouse_wheel(c, -2.5, 18, 19);
        fail_after = -1;
        double x, y; assert(fvm_input_get_pointer(c, &x, &y) == FVM_OK);
        if (status == FVM_OK) { done = true; assert(x == 18 && y == 19); }
        else {
            assert(status == FVM_OUT_OF_MEMORY);
            if (x == 3) { assert(y == 4); ++before; }
            else { assert(x == 18 && y == 19); ++after; }
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->mouse_dispatching);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
    }
    assert(done && before && after);
    fail_after = 0; int removed = 0;
    assert(fvm_input_remove_mouse_listener(c, listener, &removed) == FVM_OK && removed);
    fail_after = -1;
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
