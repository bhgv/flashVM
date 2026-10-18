/* Class activation keeps edited flags/values, accessors, deleted slots and primitive metadata. */
#include "static_activation_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned field = 0; field < 8; ++field)
    for (unsigned kind = 0; kind < 5; ++kind) {
        if (field >= 5 && kind == 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *klass = static_class(c, field < 5 ? 0 : field - 4); static_prototype(c, klass, kind == 4 ? 0 : 4);
        FVM_Value original, value; assert(fvm_object_get(c, klass, static_fields[field], &original) == FVM_OK);
        if (!kind) { assert(fvm_object_set_property_flags(c, klass, static_fields[field], 0, 7) == FVM_OK && fvm_object_set_property_flags(c, klass, static_fields[field], 2, 0) == FVM_OK); }
        if (kind == 1) { fvm_value_number(&original, 81); assert(fvm_object_set(c, klass, static_fields[field], &original) == FVM_OK); }
        if (kind == 2 || kind == 3) { int removed = 0; assert(fvm_object_delete(c, klass, static_fields[field], &removed) == FVM_OK && removed); }
        if (kind == 3) assert(fvm_object_define_accessor_rooted(c, klass, static_fields[field], static_getter, 0, 0, 0) == FVM_OK);
        if (kind == 4) { FVM_Object *meta = 0; assert(fvm_object_create(c, &meta) == FVM_OK); original.metatable = meta; assert(fvm_object_set(c, klass, static_fields[field], &original) == FVM_OK); }
        unsigned expected = 0; assert(fvm_object_property_flags(c, klass, static_fields[field], &expected) == FVM_OK);
        static_reads = 0; static_load(c, version); assert(!static_reads && fvm_gc_collect(c, 0) == FVM_OK); math_flag(c, klass, static_fields[field], expected);
        if (kind == 2) { int own = 1; assert(fvm_object_has_own(c, klass, static_fields[field], &own) == FVM_OK && !own); }
        else if (kind != 3) { assert(fvm_object_get(c, klass, static_fields[field], &value) == FVM_OK && value.type == original.type && value.metatable == original.metatable); if (value.type == FVM_VALUE_NUMBER) assert(value.number == original.number || (value.number != value.number && original.number != original.number)); }
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
