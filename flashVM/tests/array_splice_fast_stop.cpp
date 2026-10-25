/* Real OS workers can cancel either previously uncharged native splice staging phase. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"
#include "../src/standard/array_generic.hpp"
#include "flashvm/worker.h"


static FVM_Result fast_splice_worker(FVM_Context *c, const FVM_Value *input, FVM_Value *out, void *)
{
    const unsigned code = (unsigned)input->number, version = 4 + code / 2, insertion = code % 2;
    version_load(c, version);
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *array = 0;
    if (!(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value, function;
    fvm_value_number(&value, insertion ? 0 : 80);
    if (!(fvm_object_set(c, array, "length", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!insertion)
    {
        for (unsigned i = 0; i < 80; ++i)
        {
            fvm_value_number(&value, i + 11);
            if (!(fvm_array_store_own(c, array, i, value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(!fvm_array_needs_generic(array)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, array, "splice", &function) == FVM_OK && fvm_gc_pin(c, function.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_detail::vector<FVM_Value> args;
    for (unsigned i = 0; i < (insertion ? 82U : 2U); ++i)
    {
        fvm_value_number(&value, i < 2 ? 0 : i + 10);
        args.push_back(value);
    }
    FVM_Worker *worker = 0;
    if (!(fvm_worker_current(c, &worker) == FVM_OK && worker))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long before = 0, after = 0;
    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_gc_object_count(c, &before) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_worker_stop(worker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, array, "splice", (unsigned)args.size(), args.begin(), &value) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(array->resource_result.number == (insertion ? 0 : 80) && array->indexed.size() == (insertion ? 0U : 80U)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth && !c->calling_owner))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_gc_object_count(c, &after) == FVM_OK && before == after))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cfg.instruction_limit = 0;
    for (unsigned i = 0; i < 10; ++i)
    {
        FVM_Value input, out;
        fvm_value_number(&input, i);
        FVM_Worker *worker = 0;
        if (!(fvm_worker_run(&cfg, fast_splice_worker, &input, 0, &worker) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_worker_join(worker, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(worker);
    }
}
