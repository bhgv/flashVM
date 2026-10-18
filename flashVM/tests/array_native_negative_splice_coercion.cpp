/* Deletion coercion can change native length after signed bounds have been captured. */
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"
static unsigned conversions;
static FVM_Result signed_deletion(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    ++conversions; FVM_Value value; fvm_value_number(&value, 2);
    assert(fvm_object_set(c, (FVM_Object *)raw, "length", &value) == FVM_OK);
    fvm_value_number(out, 1); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 7; ++kind)
    for (unsigned protected_slot = 0; protected_slot < 2; ++protected_slot) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value value, args[4], out; fvm_value_number(&value, -3); FVM_Object *array = 0, *counter = 0;
        assert(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &counter) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, counter, "valueOf", signed_deletion, array, array) == FVM_OK);
        fvm_value_number(&value, 81); assert(fvm_array_store_own(c, array, 0, value) == FVM_OK);
        fvm_value_number(&value, 82); assert(fvm_array_store_own(c, array, 1, value) == FVM_OK);
        fvm_value_number(&value, 84); assert(fvm_array_store_own(c, array, 4, value) == FVM_OK);
        if (protected_slot) assert(fvm_object_set_property_flags(c, array, "0", 2, 0) == FVM_OK);
        fvm_value_number(args, kind == 1 ? -1 : kind == 5 ? -2147483648.0 : 0);
        args[1] = class_value(counter); fvm_value_number(args + 2, 10); fvm_value_number(args + 3, 11);
        unsigned argc = 4;
        if (kind == 2) fvm_value_undefined(args + 1);
        if (kind == 3) fvm_value_number(args + 1, -1);
        if (kind == 4 || kind == 5) argc = 1;
        if (kind == 6) argc = 0;
        conversions = 0;
        FVM_Result status = fvm_object_call(c, array, "splice", argc, args, &out);
        assert(status == (kind == 5 ? FVM_UNSUPPORTED : FVM_OK));
        assert(conversions == (kind < 2 ? 1U : 0U));
        if (kind == 0 || kind == 4) {
            assert(out.type == FVM_VALUE_OBJECT && fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == 0);
        } else if (kind != 5) assert(out.type == FVM_VALUE_UNDEFINED);
        assert(fvm_object_get(c, array, "length", &value) == FVM_OK && value.number == (kind == 0 ? -1 : kind == 1 ? 2 : -3));
        int own = 0; assert(fvm_object_has_own(c, array, "0", &own) == FVM_OK && !!own == (kind != 0 || protected_slot));
        assert(fvm_object_has_own(c, array, "1", &own) == FVM_OK && !!own == (kind != 0));
        assert(fvm_object_get(c, array, "4", &value) == FVM_OK && value.number == 84);
        if (!kind) {
            assert(fvm_object_get(c, array, "-3", &value) == FVM_OK && value.number == 10);
            assert(fvm_object_get(c, array, "-2", &value) == FVM_OK && value.number == 11);
        }
        FVM_Object *budget = 0; fvm_value_number(&value, -3);
        assert(fvm_construct(c, "Array", 1, &value, &budget) == FVM_OK && fvm_gc_pin(c, budget) == FVM_OK);
        FVM_Value many[6]; fvm_value_number(many, 0); fvm_value_number(many + 1, 0);
        for (unsigned i = 2; i < 6; ++i) fvm_value_number(many + i, 10 + i);
        const unsigned long limit = rt->config.instruction_limit; rt->config.instruction_limit = 1;
        assert(fvm_object_call(c, budget, "splice", 6, many, &out) == FVM_ERROR);
        rt->config.instruction_limit = limit;
        assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_call(c, budget, "splice", 6, many, &out) == FVM_OK);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
