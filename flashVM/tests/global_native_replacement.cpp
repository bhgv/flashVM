/* Replacing native globals releases unused cached captures and preserves saved Function values. */
#include "../src/core/runtime.hpp"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/standard.h"
#include <cassert>
static FVM_Value replacement_value(FVM_Object *object)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = object; return value; }
static bool replacement_alive(FVM_Context *c, FVM_Object *object)
{ for (size_t i = 0; i < c->objects.size(); ++i) if (c->objects[i] == object) return true; return false; }
static FVM_Result replacement_native(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *userdata)
{ return fvm_object_get(c, (FVM_Object *)userdata, "marker", out); }
static FVM_Object *replacement_capture(FVM_Context *c, double marker)
{
    FVM_Object *object = 0; assert(fvm_object_create(c, &object) == FVM_OK); FVM_Value value = replacement_value(object);
    assert(fvm_object_set(c, object, "self", &value) == FVM_OK); fvm_value_number(&value, marker); assert(fvm_object_set(c, object, "marker", &value) == FVM_OK); return object;
}
int main()
{
    FVM_Config config; FVM_Runtime *runtime = 0; assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) for (unsigned alias = 0; alias < 2; ++alias) for (unsigned saved = 0; saved < 2; ++saved) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(runtime, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        const char *original = "NativeMix", *updated = alias ? "nATIVEmIX" : original;
        FVM_Object *old_capture = replacement_capture(c, 1); FVM_Value old_function, new_function, out;
        assert(fvm_register_function_rooted(c, original, replacement_native, old_capture, old_capture) == FVM_OK && fvm_get_global(c, original, &old_function) == FVM_OK);
        if (saved) assert(fvm_set_global(c, "savedFunction", &old_function) == FVM_OK);
        FVM_Object *new_capture = replacement_capture(c, 2);
        assert(fvm_register_function_rooted(c, updated, replacement_native, new_capture, new_capture) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        bool separate = alias && (!version || version >= 7);
        assert(replacement_alive(c, old_capture) == (!!saved || separate) && replacement_alive(c, new_capture));
        assert(fvm_get_global(c, updated, &new_function) == FVM_OK && new_function.type == FVM_VALUE_OBJECT);
        assert(fvm_function_invoke(c, new_function.object, c->global, 0, 0, &out) == FVM_OK && out.number == 2);
        assert(fvm_register_function_rooted(c, updated, replacement_native, new_capture, new_capture) == FVM_OK && fvm_get_global(c, updated, &out) == FVM_OK && out.object == new_function.object);
        assert(fvm_get_global(c, original, &out) == FVM_OK && (separate ? out.object != new_function.object : out.object == new_function.object));
        if (saved) { assert(fvm_get_global(c, "savedFunction", &out) == FVM_OK && out.object == old_function.object); assert(fvm_function_invoke(c, out.object, c->global, 0, 0, &out) == FVM_OK && out.number == 1); }
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime); return 0;
}
