/* Literal signed decimal names grow loaded Arrays with wrapping Int32 semantics. */
#include "property_version_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *names[] = {"01", "+1", " \t\r\n\v\f1", "-1", "4294967295", "4294967297", "18446744073709551617", "2147483647", "2147483648", "1 ", "1.0", "0x1", "+", "--1", "-0", "000"};
    const double expected[] = {2,2,2,0,0,2,2,-2147483648.0,0,0,0,0,0,0,1,1};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned i = 0; i < sizeof(names)/sizeof(names[0]); ++i)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0; assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK);
        FVM_Value target = class_value(array), input, out; fvm_value_number(&input, 73);
        assert(fvm_set_global(c, "namedTarget", &target) == FVM_OK);
        if (!route) assert(fvm_object_set(c, array, names[i], &input) == FVM_OK);
        else {
            Program code; code.variable("namedTarget"); code.text(names[i]); code.number(73); code.op(0x4f); code.number(1); code.finish();
            out = boxing_run(c, code, route - 1, FVM_OK);
        }
        assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == expected[i]);
        assert(fvm_object_get(c, array, names[i], &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 73);
        if (i < 3) {
            assert(fvm_table_get_index(c, array, 1, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        }
        if (!route) {
            unsigned char *wire = 0; size_t size = 0;
            assert(fvm_value_serialize(c, &target, &wire, &size) == FVM_OK);
            FVM_Context *other = 0; assert(fvm_context_create(rt, &other) == FVM_OK);
            assert(fvm_value_deserialize(other, wire, size, &out) == FVM_OK); fvm_serialized_free(wire);
            FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
            assert(fvm_object_get(other, copy, "length", &out) == FVM_OK && out.number == expected[i]);
            assert(fvm_object_get(other, copy, names[i], &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 73);
            fvm_context_destroy(other);
        }
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    /* Overflow truncates before readonly index handling, even with readonly length. */
    for (unsigned fixed = 0; fixed < 2; ++fixed) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, 8); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value args[2], out; fvm_value_number(args, 10); fvm_value_number(args + 1, 11);
        FVM_Object *a = 0; assert(fvm_construct(c, "Array", 2, args, &a) == FVM_OK);
        assert(fvm_object_set_property_flags(c, a, "1", 2, 0) == FVM_OK);
        assert(fvm_object_set_property_flags(c, a, "2147483647", 4, 0) == FVM_OK);
        if (fixed) assert(fvm_object_set_property_flags(c, a, "length", 4, 0) == FVM_OK);
        assert(fvm_table_set_index(c, a, 2147483647UL, args) == FVM_OK);
        assert(fvm_object_get(c, a, "length", &out) == FVM_OK && out.number == (fixed ? 2 : -2147483648.0));
        int own = 0; assert(fvm_object_has_own(c, a, "0", &own) == FVM_OK && !own);
        assert(fvm_object_has_own(c, a, "1", &own) == FVM_OK && own);
        assert(fvm_object_has_own(c, a, "2147483647", &own) == FVM_OK && !own);
        fvm_context_destroy(c);
    }
    /* Host-only embedding retains canonical Uint32 storage semantics. */
    {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *a = 0; FVM_Value value, out; assert(fvm_construct(c, "Array", 0, 0, &a) == FVM_OK);
        fvm_value_number(&value, 81);
        assert(fvm_object_set(c, a, "+1", &value) == FVM_OK && fvm_object_set(c, a, "01", &value) == FVM_OK);
        assert(fvm_object_get(c, a, "length", &out) == FVM_OK && out.number == 0);
        assert(fvm_table_set_index(c, a, 2147483647UL, &value) == FVM_OK);
        assert(fvm_object_get(c, a, "length", &out) == FVM_OK && out.number == 2147483648.0);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
