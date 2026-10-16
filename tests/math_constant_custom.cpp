/* First-load correction skips custom flags, numbers, metadata and deleted slots. */
#include "math_constant_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 4; ++kind) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Value math, original, out; assert(fvm_get_global(c, "Math", &math) == FVM_OK && fvm_object_get(c, math.object, "LOG10E", &original) == FVM_OK);
        if (!kind) assert(fvm_object_set_property_flags(c, math.object, "LOG10E", 1, 0) == FVM_OK);
        if (kind == 1) { fvm_value_number(&original, 42); assert(fvm_object_set(c, math.object, "LOG10E", &original) == FVM_OK); }
        if (kind == 2) { FVM_Object *meta = 0; assert(fvm_object_create(c, &meta) == FVM_OK); original.metatable = meta; assert(fvm_object_set(c, math.object, "LOG10E", &original) == FVM_OK); }
        if (kind == 3) { int removed = 0; assert(fvm_object_delete(c, math.object, "LOG10E", &removed) == FVM_OK && removed); }
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        if (kind == 3) { int own = 1; assert(fvm_object_has_own(c, math.object, "LOG10E", &own) == FVM_OK && !own); }
        else { assert(fvm_object_get(c, math.object, "LOG10E", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == original.number && out.metatable == original.metatable); math_flag(c, math.object, "LOG10E", !kind ? 1U : 0U); }
        /* Already loaded user edits are retained on another load as well. */
        if (kind != 3) { assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_object_get(c, math.object, "LOG10E", &out) == FVM_OK && out.number == original.number && out.metatable == original.metatable); }
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
