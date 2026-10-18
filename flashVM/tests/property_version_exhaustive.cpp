/* Exhaustive UInt16 attributes: independent masks, ordered updates and write protection. */
#include "property_version_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0;
        assert(fvm_context_create(rt, &c) == FVM_OK);
        version_load(c, version);
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *child = 0, *parent = 0;
        assert(fvm_construct(c, "Object", 0, 0, &child) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK);
        assert(fvm_object_set_prototype(c, child, parent) == FVM_OK);
        FVM_Value value, out;
        fvm_value_number(&value, 11);
        assert(fvm_object_set(c, parent, "field", &value) == FVM_OK);
        value = class_value(child);
        assert(fvm_set_global(c, "exhaustiveTarget", &value) == FVM_OK);
        for (unsigned flags = 0; flags <= 65535; ++flags) {
            version_flags(c, child, "field", 0, 65535);
            fvm_value_number(&value, 22);
            assert(fvm_object_set(c, child, "field", &value) == FVM_OK);
            version_flags(c, child, "field", flags, 65535);
            unsigned actual = 0;
            assert(fvm_object_property_flags(c, child, "field", &actual) == FVM_OK && actual == flags);
            assert(fvm_object_get(c, child, "field", &out) == FVM_OK);
            assert(out.type == FVM_VALUE_NUMBER && out.number == ((flags & version_masks[version - 4]) ? 11 : 22));
            int own = 0;
            assert(fvm_object_has_own(c, child, "field", &own) == FVM_OK && own);
            /* Rotation makes every source bit meet both overlapping and disjoint set/clear masks. */
            const unsigned clear = ((flags << 5) | (flags >> 11)) & 65535;
            const unsigned set = flags ^ 0xa55a;
            const unsigned expected = (flags & ~clear) | set;
            version_flags(c, child, "field", set, clear);
            assert(fvm_object_property_flags(c, child, "field", &actual) == FVM_OK && actual == expected);
            fvm_value_number(&value, 33);
            assert(fvm_object_set(c, child, "field", &value) == FVM_OK);
            const unsigned after_write = (expected & 4) ? expected : expected & 7;
            assert(fvm_object_property_flags(c, child, "field", &actual) == FVM_OK && actual == after_write);
            assert(fvm_object_get(c, child, "field", &out) == FVM_OK);
            assert(out.type == FVM_VALUE_NUMBER && out.number == ((after_write & version_masks[version - 4]) ? 11 : ((expected & 4) ? 22 : 33)));
            /* Inspect the retained value even when readonly plus version flags hid it. */
            version_flags(c, child, "field", 0, 65535);
            assert(fvm_object_get(c, child, "field", &out) == FVM_OK);
            assert(out.type == FVM_VALUE_NUMBER && out.number == ((expected & 4) ? 22 : 33));
            version_flags(c, child, "field", flags, 65535);
            int removed = 0;
            assert(fvm_object_delete(c, child, "field", &removed) == FVM_OK);
            assert(!!removed == !(flags & 2));
            assert(fvm_object_has_own(c, child, "field", &own) == FVM_OK && !!own == !!(flags & 2));
            if (!(flags & 2)) {
                assert(fvm_object_property_flags(c, child, "field", &actual) == FVM_OK && !actual);
                assert(fvm_object_get(c, child, "field", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 11);
            }
            if (!(flags & 4095)) assert(fvm_gc_collect(c, 0) == FVM_OK);
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
