/* Budget-limited staging stays unpublished through OOM boundaries, then succeeds on retry. */
#define main fast_splice_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned insertion = 0; insertion < 2; ++insertion) {
        bool saw_oom = false, reached_budget = false;
        for (long boundary = 0; boundary < 1024 && !reached_budget; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
            FVM_Object *array = 0; assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
            FVM_Value value, out; fvm_value_number(&value, insertion ? 0 : 12); assert(fvm_object_set(c, array, "length", &value) == FVM_OK);
            if (!insertion) for (unsigned i = 0; i < 12; ++i) { assert(fvm_value_string_bytes(&value, "kept\0tail", 9) == FVM_OK && fvm_array_store_own(c, array, i, value) == FVM_OK); }
            fvm_detail::vector<FVM_Value> args; fvm_value_number(&value, 0); args.push_back(value); args.push_back(value);
            if (insertion) for (unsigned i = 0; i < 12; ++i) { assert(fvm_value_string_bytes(&value, "new\0tail", 8) == FVM_OK); args.push_back(value); }
            const unsigned long limit = rt->config.instruction_limit; rt->config.instruction_limit = 4;
            allowance = boundary; FVM_Result status = fvm_object_call(c, array, "splice", (unsigned)args.size(), args.begin(), &out); allowance = -1;
            rt->config.instruction_limit = limit;
            assert(status == FVM_OUT_OF_MEMORY || status == FVM_ERROR);
            if (status == FVM_OUT_OF_MEMORY) saw_oom = true; else reached_budget = true;
            assert(array->resource_result.number == (insertion ? 0 : 12) && array->indexed.size() == (insertion ? 0U : 12U));
            assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth && !c->calling_owner);
            assert(fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_object_call(c, array, "splice", (unsigned)args.size(), args.begin(), &out) == FVM_OK && out.type == FVM_VALUE_OBJECT);
            assert(array->resource_result.number == 12 && array->indexed.size() == 12);
            assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_table_get_index(c, array, 0, &value) == FVM_OK);
            assert(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == (insertion ? 8U : 9U) && !std::memcmp(value.string, insertion ? "new\0tail" : "kept\0tail", insertion ? 8 : 9));
            fvm_context_destroy(c);
        }
        assert(saw_oom && reached_budget);
    }
    fvm_runtime_destroy(rt);
}
