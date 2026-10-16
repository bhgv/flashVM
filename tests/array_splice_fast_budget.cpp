/* Fast native splice must charge stored-entry staging and every inserted argument. */
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"
#include "../src/standard/array_generic.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        for (unsigned kind = 0; kind < 3; ++kind)
        for (unsigned route = 0; route < (version ? 5U : 2U); ++route) {
            if (route >= 3 && !fvm_jit_available()) continue;
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); if (version) version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
            FVM_Object *array = 0; FVM_Value value, function, out;
            assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
            const unsigned long length = kind == 0 ? 80 : kind == 2 ? 6 : 0;
            fvm_value_number(&value, length); assert(fvm_object_set(c, array, "length", &value) == FVM_OK);
            for (unsigned long i = 0; i < length; ++i) {
                if (kind == 2 && i != 0 && i != 5) continue;
                fvm_value_number(&value, i + 11); assert(fvm_array_store_own(c, array, i, value) == FVM_OK);
            }
            assert(!fvm_array_needs_generic(array));
            value = class_value(array); assert(fvm_set_global(c, "fastSpliceTarget", &value) == FVM_OK);
            assert(fvm_object_get(c, array, "splice", &function) == FVM_OK && fvm_gc_pin(c, function.object) == FVM_OK);
            fvm_detail::vector<FVM_Value> args;
            for (unsigned i = 0; i < (kind == 1 ? 82U : 2U); ++i) {
                FVM_Value arg; fvm_value_number(&arg, i < 2 ? (kind == 2 ? (i ? 3 : 1) : 0) : i + 10); args.push_back(arg);
            }
            const unsigned long old_limit = rt->config.instruction_limit;
            rt->config.instruction_limit = !version ? 1 : kind == 0 ? 64 : kind == 1 ? (route < 2 ? 64 : 120) : route < 2 ? 4 : 1000;
            const FVM_Result expected = !version || (kind == 2 && route >= 2) ? FVM_OK : FVM_ERROR;
            unsigned long before = 0, after = 0; assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_gc_object_count(c, &before) == FVM_OK);
            if (!route) assert(fvm_object_call(c, array, "splice", (unsigned)args.size(), args.begin(), &out) == expected);
            else if (route == 1) assert(fvm_function_invoke(c, function.object, array, (unsigned)args.size(), args.begin(), &out) == expected);
            else {
                Program code; for (unsigned i = (unsigned)args.size(); i; --i) code.number((unsigned)args[i-1].number);
                code.number((unsigned)args.size()); code.variable("fastSpliceTarget"); code.text("splice"); code.op(0x52); code.finish();
                out = boxing_run(c, code, route - 2, expected);
            }
            rt->config.instruction_limit = old_limit;
            assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth && !c->calling_owner);
            if (expected == FVM_ERROR) {
                assert(array->resource_result.number == length && array->indexed.size() == (kind == 2 ? 2U : length));
                for (fvm_detail::map<unsigned long, FVM_Value>::iterator i = array->indexed.begin(); i != array->indexed.end(); ++i) assert(i->second.type == FVM_VALUE_NUMBER && i->second.number == i->first + 11);
                assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_gc_object_count(c, &after) == FVM_OK);
                if (route < 2) assert(before == after);
                assert(fvm_object_call(c, array, "splice", (unsigned)args.size(), args.begin(), &out) == FVM_OK);
                assert(c->avm1_steps == (kind == 2 ? 5U : 80U));
            }
            assert(out.type == FVM_VALUE_OBJECT && fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == (kind == 2 ? 3 : 0));
            assert(array->resource_result.number == (kind == 0 ? 80 : kind == 1 ? 80 : 3));
            assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
