/* Native focus selection, detached ancestry, rooted callback snapshots and failure policy. */
#include "../src/core/runtime.hpp"
#include "../src/standard/movieclip_scene.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstring>

struct Host
{
    FVM_Object *first, *next, *listener;
    unsigned order[16], count;
    bool mutate, fail;
};
static FVM_Result receive(FVM_Context *c, FVM_Object *self, unsigned argc,
                          const FVM_Value *, FVM_Value *out, void *raw)
{
    Host *h = (Host *)raw;
    assert(!argc && c->keyboard_dispatching && c->keyboard.down[FVM_KEY_LEFT]);
    h->order[h->count++] = self == h->first ? 1 : self == h->next ? 2 : 3;
    if (h->mutate && self == h->first)
    {
        assert(fvm_input_set_keyboard_focus(c, h->next) == FVM_OK);
        int removed = 0;
        assert(fvm_input_remove_key_listener(c, h->listener, &removed) == FVM_OK && removed);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        h->mutate = false;
    }
    fvm_value_undefined(out);
    return h->fail ? FVM_UNSUPPORTED : FVM_OK;
}
static FVM_Object *child(FVM_Context *c, FVM_Object *parent, const char *name)
{
    FVM_Value args[2], out;
    fvm_value_string(args, name); fvm_value_number(args + 1, 1);
    assert(fvm_object_call(c, parent, "createEmptyMovieClip", 2, args, &out) == FVM_OK);
    return out.object;
}
int main()
{
    FVM_Config config; FVM_Runtime *rt = 0; FVM_Context *c = 0, *other = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    Host h; std::memset(&h, 0, sizeof(h));
    assert(fvm_movieclip_create(c, "first", 1, &h.first) == FVM_OK);
    assert(fvm_input_set_keyboard_focus(c, h.first) == FVM_OK);
    assert(fvm_movieclip_create(c, "next", 1, &h.next) == FVM_OK);
    assert(fvm_gc_pin(c, h.next) == FVM_OK);
    assert(fvm_object_create(c, &h.listener) == FVM_OK);
    assert(fvm_input_add_key_listener(c, h.listener) == FVM_OK);
    FVM_Object *targets[] = {h.first, h.next, h.listener};
    for (unsigned i = 0; i < 3; ++i)
    {
        assert(fvm_object_define_method_rooted(c, targets[i], "onKeyDown", receive, &h, 0) == FVM_OK);
        targets[i]->methods["onKeyDown"].collector_entry = true;
    }
    FVM_Object *selected = 0;
    assert(fvm_input_get_keyboard_focus(c, &selected) == FVM_OK && selected == h.first);
    assert(fvm_input_get_keyboard_focus(c, 0) == FVM_INVALID_ARGUMENT);
    assert(fvm_input_set_keyboard_focus(c, h.listener) == FVM_INVALID_ARGUMENT);
    assert(fvm_input_set_keyboard_focus(other, h.first) == FVM_INVALID_ARGUMENT);
    assert(fvm_input_get_keyboard_focus(c, &selected) == FVM_OK && selected == h.first);
    FVM_Value ctor, prototype;
    assert(fvm_get_global(c, "MovieClip", &ctor) == FVM_OK);
    assert(fvm_object_get(c, ctor.object, "prototype", &prototype) == FVM_OK);
    assert(fvm_input_set_keyboard_focus(c, prototype.object) == FVM_INVALID_ARGUMENT);
    /* Only the focus root protects first before a real collection. */
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    FVM_KeyboardState state; std::memset(&state, 0, sizeof(state)); state.struct_size = sizeof(state);
    state.last_code = FVM_KEY_LEFT; state.down[FVM_KEY_LEFT] = 1;
    h.mutate = true;
    assert(fvm_input_dispatch_keyboard(c, &state, 1) == FVM_OK);
    assert(h.count == 2 && h.order[0] == 1 && h.order[1] == 3);
    assert(fvm_input_get_keyboard_focus(c, &selected) == FVM_OK && selected == h.next);
    assert(fvm_input_dispatch_keyboard(c, &state, 1) == FVM_OK);
    assert(h.count == 3 && h.order[2] == 2);
    assert(fvm_input_add_key_listener(c, h.next) == FVM_OK);
    assert(fvm_input_dispatch_keyboard(c, &state, 1) == FVM_OK);
    assert(h.count == 5 && h.order[3] == 2 && h.order[4] == 2);
    h.fail = true;
    assert(fvm_input_dispatch_keyboard(c, &state, 1) == FVM_UNSUPPORTED);
    assert(h.count == 6 && !c->keyboard_dispatching && !c->gc_frames);
    h.fail = false;
    int removed = 0;
    assert(fvm_input_remove_key_listener(c, h.next, &removed) == FVM_OK && removed);
    /* Removing an ancestor invalidates focus even if the child retains its script ancestry. */
    FVM_Object *parent = child(c, h.next, "parent");
    FVM_Object *nested = child(c, parent, "nested");
    assert(fvm_input_set_keyboard_focus(c, nested) == FVM_OK);
    assert(fvm_movieclip_scene_remove(c, parent) == FVM_OK);
    assert(fvm_input_get_keyboard_focus(c, &selected) == FVM_OK && !selected);
    assert(fvm_input_set_keyboard_focus(c, nested) == FVM_INVALID_ARGUMENT);
    assert(fvm_input_set_keyboard_focus(c, h.next) == FVM_OK);
    assert(fvm_movieclip_scene_remove(c, h.next) == FVM_OK);
    assert(fvm_input_dispatch_keyboard(c, &state, 1) == FVM_OK && h.count == 6);
    assert(fvm_input_get_keyboard_focus(c, &selected) == FVM_OK && !selected);
    assert(fvm_input_set_keyboard_focus(c, 0) == FVM_OK);
    assert(fvm_input_reset_keyboard(c) == FVM_OK);
    assert(fvm_gc_unpin(c, h.next) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
