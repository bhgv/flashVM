/* Sparse splice charges stored records, including ignored post-conversion tails. */
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"
#include "../src/standard/array_generic.hpp"
static FVM_Result extend_splice(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    FVM_Object *array = (FVM_Object *)raw; FVM_Value value; fvm_value_number(&value, 1000000);
    assert(fvm_object_set(c, array, "length", &value) == FVM_OK);
    fvm_value_number(&value, 91); assert(fvm_array_store_own(c, array, 999999, value) == FVM_OK);
    fvm_value_number(out, 0); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0; FVM_Value value, args[2], out;
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
        fvm_value_number(&value, 1000000); assert(fvm_object_set(c, array, "length", &value) == FVM_OK);
        fvm_value_number(&value, 81); assert(fvm_array_store_own(c, array, 0, value) == FVM_OK);
        fvm_value_number(&value, 91); assert(fvm_array_store_own(c, array, 999999, value) == FVM_OK);
        assert(!fvm_array_needs_generic(array)); fvm_value_number(args, 0); fvm_value_number(args + 1, 0);
        const unsigned long limit = rt->config.instruction_limit; rt->config.instruction_limit = 1;
        assert(fvm_object_call(c, array, "splice", 2, args, &out) == FVM_ERROR);
        assert(array->resource_result.number == 1000000 && array->indexed.size() == 2);
        rt->config.instruction_limit = 2;
        assert(fvm_object_call(c, array, "splice", 2, args, &out) == FVM_OK && c->avm1_steps == 2);
        assert(out.type == FVM_VALUE_OBJECT && out.object->resource_result.number == 0);
        assert(array->resource_result.number == 1000000 && array->indexed.size() == 2);
        rt->config.instruction_limit = limit;
        FVM_Object *changed = 0, *counter = 0; assert(fvm_construct(c, "Array", 0, 0, &changed) == FVM_OK && fvm_gc_pin(c, changed) == FVM_OK);
        fvm_value_number(&value, 2); assert(fvm_object_set(c, changed, "length", &value) == FVM_OK);
        fvm_value_number(&value, 81); assert(fvm_array_store_own(c, changed, 0, value) == FVM_OK && fvm_array_store_own(c, changed, 1, value) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &counter) == FVM_OK && fvm_gc_pin(c, counter) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, counter, "valueOf", extend_splice, changed, changed) == FVM_OK);
        args[1] = class_value(counter); rt->config.instruction_limit = 2;
        assert(fvm_object_call(c, changed, "splice", 2, args, &out) == FVM_ERROR && c->avm1_steps == 2);
        assert(changed->resource_result.number == 1000000 && changed->indexed.size() == 3);
        assert(fvm_table_get_index(c, changed, 999999, &value) == FVM_OK && value.number == 91);
        rt->config.instruction_limit = limit;
        assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_call(c, changed, "splice", 2, args, &out) == FVM_OK);
        assert(changed->resource_result.number == 1000000 && changed->indexed.size() == 3);
        assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth && !c->calling_owner);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
