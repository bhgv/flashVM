/* Budget-limited staging stays unpublished through OOM boundaries, then succeeds on retry. */
#define main fast_splice_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned insertion = 0; insertion < 2; ++insertion)
        {
            bool saw_oom = false, reached_budget = false;
            for (long boundary = 0; boundary < 1024 && !reached_budget; ++boundary)
            {
                FVM_Context *c = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                version_load(c, version);
                if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
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
                FVM_Value value, out;
                fvm_value_number(&value, insertion ? 0 : 12);
                if (!(fvm_object_set(c, array, "length", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!insertion)
                {
                    for (unsigned i = 0; i < 12; ++i)
                    {
                        if (!(fvm_value_string_bytes(&value, "kept\0tail", 9) == FVM_OK &&
                              fvm_array_store_own(c, array, i, value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                fvm_detail::vector<FVM_Value> args;
                fvm_value_number(&value, 0);
                args.push_back(value);
                args.push_back(value);
                if (insertion)
                {
                    for (unsigned i = 0; i < 12; ++i)
                    {
                        if (!(fvm_value_string_bytes(&value, "new\0tail", 8) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        args.push_back(value);
                    }
                }
                const unsigned long limit = rt->config.instruction_limit;
                rt->config.instruction_limit = 4;
                allowance = boundary;
                FVM_Result status = fvm_object_call(c, array, "splice", (unsigned)args.size(), args.begin(), &out);
                allowance = -1;
                rt->config.instruction_limit = limit;
                if (!(status == FVM_OUT_OF_MEMORY || status == FVM_ERROR))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (status == FVM_OUT_OF_MEMORY)
                {
                    saw_oom = true;
                }
                else
                {
                    reached_budget = true;
                }
                if (!(array->resource_result.number == (insertion ? 0 : 12) &&
                      array->indexed.size() == (insertion ? 0U : 12U)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth && !c->calling_owner))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_call(c, array, "splice", (unsigned)args.size(), args.begin(), &out) == FVM_OK &&
                      out.type == FVM_VALUE_OBJECT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(array->resource_result.number == 12 && array->indexed.size() == 12))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_table_get_index(c, array, 0, &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == (insertion ? 8U : 9U) &&
                      !std::memcmp(value.string, insertion ? "new\0tail" : "kept\0tail", insertion ? 8 : 9)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
            if (!(saw_oom && reached_budget))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
