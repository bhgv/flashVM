/* Own membership and enumeration flags obey loaded Flash name casing without getters. */
#include "flashvm/standard.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include <cassert>
#include <cstring>
static unsigned touched;
static FVM_Result forbidden(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ ++touched; return FVM_ERROR; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *object = 0, *parent = 0;
        assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_gc_pin(c, object) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK && fvm_object_set_prototype(c, object, parent) == FVM_OK);
        FVM_Value value, arg, out; fvm_value_number(&value, 17);
        assert(fvm_object_set(c, parent, "Inherited", &value) == FVM_OK);
        const char *stored[] = {"MiXeD", "\xc3\x84rea", "N\0M", "\xed\xa0\x80" "A", "Getter", "Method"};
        const char *query[] = {"mixed", "\xc3\xa4rea", "n\0m", "\xed\xa0\x80" "a", "getter", "method"};
        const size_t sizes[] = {5, 5, 3, 4, 6, 6};
        touched = 0;
        for (unsigned i = 0; i < 6; ++i) {
            if (i == 4) assert(fvm_object_define_accessor_rooted(c, object, stored[i], forbidden, 0, 0, 0) == FVM_OK);
            else if (i == 5) assert(fvm_object_define_method_rooted(c, object, stored[i], forbidden, 0, 0) == FVM_OK);
            else assert(fvm_object_set_bytes(c, object, stored[i], sizes[i], &value) == FVM_OK);
            assert(fvm_object_set_property_flags_bytes(c, object, stored[i], sizes[i], FVM_PROPERTY_DONT_ENUM, 0) == FVM_OK);
            const bool folded = version && version <= 6;
            int own = 0; unsigned flags = 0;
            assert(fvm_object_has_own_bytes(c, object, query[i], sizes[i], &own) == FVM_OK && !!own == folded);
            assert(fvm_object_property_flags_bytes(c, object, query[i], sizes[i], &flags) == FVM_OK && flags == (folded ? FVM_PROPERTY_DONT_ENUM : 0));
            fvm_value_string_bytes(&arg, query[i], sizes[i]);
            assert(fvm_object_call(c, object, "hasOwnProperty", 1, &arg, &out) == FVM_OK && !!out.boolean == folded);
            assert(fvm_object_call(c, object, "propertyIsEnumerable", 1, &arg, &out) == FVM_OK && !out.boolean);
            assert(fvm_object_has_own_bytes(c, object, stored[i], sizes[i], &own) == FVM_OK && own);
            if (i < 4) {
                assert(fvm_object_get_bytes(c, object, query[i], sizes[i], &out) == FVM_OK);
                if (folded) assert(out.type == FVM_VALUE_NUMBER && out.number == 17); else assert(out.type == FVM_VALUE_UNDEFINED);
                FVM_Value replacement; fvm_value_number(&replacement, 23);
                assert(fvm_object_set_bytes(c, object, query[i], sizes[i], &replacement) == FVM_OK);
                assert(fvm_object_get_bytes(c, object, stored[i], sizes[i], &out) == FVM_OK && out.number == (folded ? 23 : 17));
            }
            if (i == 5) {
                FVM_Value original;
                assert(fvm_object_get(c, object, stored[i], &original) == FVM_OK && original.type == FVM_VALUE_OBJECT);
                assert(fvm_object_get(c, object, query[i], &out) == FVM_OK);
                if (folded) assert(out.type == FVM_VALUE_OBJECT && out.object == original.object); else assert(out.type == FVM_VALUE_UNDEFINED);
            }

        }
        int own = 1; assert(fvm_object_has_own(c, object, "inherited", &own) == FVM_OK && !own);
        assert(fvm_object_get(c, object, "inherited", &out) == FVM_OK);
        if (version && version <= 6) assert(out.type == FVM_VALUE_NUMBER && out.number == 17); else assert(out.type == FVM_VALUE_UNDEFINED);
        fvm_value_string(&arg, "notHere"); assert(fvm_object_call(c, object, "hasOwnProperty", 1, &arg, &out) == FVM_OK && !out.boolean);
        assert(!touched && fvm_gc_collect(c, 0) == FVM_OK);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
