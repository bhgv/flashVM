/* Explicit Selection: native eligibility, paths, mutation, callbacks and version gating. */
#include "../src/core/runtime.hpp"
#include "../src/standard/movieclip_scene.hpp"
#include "flashvm/keyboard.h"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/serialization.h"
#include <cassert>
#include <cstring>
struct Host { unsigned calls; bool remove, fail; };
static FVM_Result eligibility(FVM_Context *c, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    Host *h = (Host *)raw; ++h->calls;
    if (h->remove) assert(fvm_movieclip_scene_remove(c, self) == FVM_OK);
    if (h->fail) return FVM_UNSUPPORTED;
    fvm_value_boolean(out, 1); return FVM_OK;
}
static FVM_Result noop(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ fvm_value_undefined(out); return FVM_OK; }
static FVM_Value object(FVM_Object *o)
{ FVM_Value v; fvm_value_undefined(&v); v.type = FVM_VALUE_OBJECT; v.object = o; return v; }
static bool select(FVM_Context *c, FVM_Object *selection, const FVM_Value &input)
{
    FVM_Value out;
    assert(fvm_object_call(c, selection, "setFocus", 1, &input, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_BOOLEAN); return out.boolean != 0;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_selection_install(c) == FVM_INVALID_ARGUMENT);
    assert(fvm_standard_install(c) == FVM_OK && fvm_standard_selection_install(c) == FVM_OK);
    FVM_Value ns, out, value;
    assert(fvm_get_global(c, "Selection", &ns) == FVM_OK);
    assert(fvm_standard_selection_install(c) == FVM_OK);
    unsigned char *bytes = 0; size_t size = 0;
    assert(fvm_value_serialize(c, &ns, &bytes, &size) == FVM_UNSUPPORTED && !bytes && !size);
    FVM_Object *root = 0;
    assert(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK);
    FVM_Value args[2]; fvm_value_string(args, "\xd0\xba\xd0\xbb\xd0\xb8\xd0\xbf"); fvm_value_number(args + 1, 1);
    assert(fvm_object_call(c, root, "createEmptyMovieClip", 2, args, &out) == FVM_OK);
    FVM_Object *clip = out.object;
    assert(!select(c, ns.object, object(clip)));
    fvm_value_boolean(&value, 1);
    assert(fvm_object_set(c, clip, "focusEnabled", &value) == FVM_OK);
    assert(select(c, ns.object, object(clip)) && c->keyboard_focus == clip);
    assert(fvm_object_call(c, ns.object, "getFocus", 0, 0, &out) == FVM_OK);
    assert(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "_level0.\xd0\xba\xd0\xbb\xd0\xb8\xd0\xbf"));
    fvm_value_number(&value, 42); assert(!select(c, ns.object, value) && c->keyboard_focus == clip);
    /* Host path lookup borrows an explicitly established native activation base. */
    c->avm1_base_clip = root; c->avm1_target_clip = root;
    fvm_value_string(&value, "/\xd0\xba\xd0\xbb\xd0\xb8\xd0\xbf"); assert(select(c, ns.object, value));
    fvm_value_string(&value, "_root.\xd0\xba\xd0\xbb\xd0\xb8\xd0\xbf"); assert(select(c, ns.object, value));
    fvm_value_string(&value, "missing"); assert(!select(c, ns.object, value) && c->keyboard_focus == clip);
    fvm_value_boolean(&value, 0); assert(fvm_object_set(c, clip, "focusEnabled", &value) == FVM_OK);
    assert(fvm_object_define_method_rooted(c, clip, "onPress", noop, 0, 0) == FVM_OK);
    assert(select(c, ns.object, object(clip)));
    Host h; std::memset(&h, 0, sizeof(h));
    assert(fvm_object_define_accessor_rooted(c, root, "focusEnabled", eligibility, 0, &h, 0) == FVM_OK);
    h.fail = true; value = object(root);
    assert(fvm_object_call(c, ns.object, "setFocus", 1, &value, &out) == FVM_UNSUPPORTED);
    assert(h.calls == 1 && c->keyboard_focus == clip);
    h.fail = false; h.remove = true;
    assert(!select(c, ns.object, value) && h.calls == 2);
    assert(fvm_object_call(c, ns.object, "getFocus", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_NULL);
    assert(!c->keyboard_focus);
    fvm_value_undefined(&value); value.type = FVM_VALUE_NULL; assert(select(c, ns.object, value));
    assert(fvm_object_call(c, ns.object, "setFocus", 0, 0, &out) == FVM_OK && out.boolean);
    fvm_value_number(&value, 1);
    assert(fvm_object_set(c, ns.object, "setFocus", &value) == FVM_OK);
    assert(fvm_standard_selection_install(c) == FVM_INVALID_ARGUMENT);
    c->avm1_base_clip = c->avm1_target_clip = 0;
    assert(fvm_gc_unpin(c, root) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
    assert(!c->gc_frames && !c->gc_native_depth);
    fvm_context_destroy(c);
    /* The SWF5 text-only contract is deliberately not installed as this MovieClip subset. */
    assert(fvm_context_create(rt, &c) == FVM_OK);
    unsigned char swf[] = {'F','W','S',5,16,0,0,0,8,0,0,12,1,0,0,0};
    assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
    assert(fvm_standard_selection_install(c) == FVM_UNSUPPORTED);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
