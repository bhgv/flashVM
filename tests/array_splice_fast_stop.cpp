/* Real OS workers can cancel either previously uncharged native splice staging phase. */
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"
#include "../src/standard/array_generic.hpp"
#include "flashvm/worker.h"
static FVM_Result fast_splice_worker(FVM_Context *c, const FVM_Value *input, FVM_Value *out, void *)
{
    const unsigned code = (unsigned)input->number, version = 4 + code / 2, insertion = code % 2;
    version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
    FVM_Object *array = 0; assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
    FVM_Value value, function; fvm_value_number(&value, insertion ? 0 : 80);
    assert(fvm_object_set(c, array, "length", &value) == FVM_OK);
    if (!insertion) for (unsigned i = 0; i < 80; ++i) { fvm_value_number(&value, i + 11); assert(fvm_array_store_own(c, array, i, value) == FVM_OK); }
    assert(!fvm_array_needs_generic(array));
    assert(fvm_object_get(c, array, "splice", &function) == FVM_OK && fvm_gc_pin(c, function.object) == FVM_OK);
    fvm_detail::vector<FVM_Value> args;
    for (unsigned i = 0; i < (insertion ? 82U : 2U); ++i) { fvm_value_number(&value, i < 2 ? 0 : i + 10); args.push_back(value); }
    FVM_Worker *worker = 0; assert(fvm_worker_current(c, &worker) == FVM_OK && worker);
    unsigned long before = 0, after = 0; assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_gc_object_count(c, &before) == FVM_OK);
    assert(fvm_worker_stop(worker) == FVM_OK);
    assert(fvm_object_call(c, array, "splice", (unsigned)args.size(), args.begin(), &value) == FVM_ERROR);
    assert(array->resource_result.number == (insertion ? 0 : 80) && array->indexed.size() == (insertion ? 0U : 80U));
    assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth && !c->calling_owner);
    assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_gc_object_count(c, &after) == FVM_OK && before == after);
    fvm_value_boolean(out, 1); return FVM_OK;
}
int main()
{
    FVM_Config cfg; assert(fvm_config_init(&cfg) == FVM_OK); cfg.instruction_limit = 0;
    for (unsigned i = 0; i < 10; ++i) {
        FVM_Value input, out; fvm_value_number(&input, i); FVM_Worker *worker = 0;
        assert(fvm_worker_run(&cfg, fast_splice_worker, &input, 0, &worker) == FVM_OK);
        assert(fvm_worker_join(worker, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN && out.boolean);
        fvm_worker_destroy(worker);
    }
}
