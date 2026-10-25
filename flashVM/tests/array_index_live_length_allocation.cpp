/* OOM at live indexed growth can be retried and preserves owned counted input metadata. */
#define main index_live_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define FVM_INDEX_LIVE_LENGTH_FIXTURE_ONLY
#include "array_index_live_length.cpp"


static FVM_Result index_live_budget(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    FVM_Value value;
    fvm_value_number(&value, 64);
    if (!(fvm_object_set(c, frame->capture, "length", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Leave less quota than the raw numeric tail; a reset between conversion and deletion is wrong. */
    while (c->avm1_steps < 49)
    {
        ++c->avm1_steps;
    }
    fvm_value_number(out, 0);
    return FVM_OK;
}


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
        for (unsigned kind = 1; kind <= 3; ++kind)
        {
            for (unsigned route = 0; route < 2; ++route)
            {
                bool failed = false, success = false;
                for (long boundary = 0; boundary < 512 && !success; ++boundary)
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
                    FVM_Value raw, input, out;
                    unsigned count = 0;
                    FVM_Object *array = array_deleted_fixture(c, kind, raw, count), *meta = 0;
                    if (!(fvm_gc_pin(c, array) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK &&
                          fvm_gc_pin(c, meta) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value target = class_value(array);
                    if (!(fvm_object_set(c, meta, "cycle", &target) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_value_string_bytes(&input, "a\0b", 3) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    input.metatable = meta;
                    allowance = boundary;
                    FVM_Result status =
                        route ? fvm_table_set_index(c, array, 3, &input) : fvm_object_set(c, array, "\t+003", &input);
                    allowance = -1;
                    if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (status == FVM_OUT_OF_MEMORY)
                    {
                        failed = true;
                        if (!(fvm_gc_collect(c, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        status = route ? fvm_table_set_index(c, array, 3, &input)
                                       : fvm_object_set(c, array, "\t+003", &input);
                        if (!(status == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(status == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        success = true;
                    }
                    if (!(fvm_gc_unpin(c, meta) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_get(c, array, route ? "3" : "\t+003", &out) == FVM_OK &&
                          out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 3 &&
                          !std::memcmp(out.string, "a\0b", 3) && out.metatable == meta))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_get(c, array, "length", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
                          out.number == 4))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
                if (!(failed && success))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        version_load(c, version);
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *array = deleted_length_array(c), *number = 0, *fn = version_function(c, 0);
        if (!(fvm_gc_pin(c, array) == FVM_OK && fvm_construct(c, "Object", 0, 0, &number) == FVM_OK &&
              fvm_function_set_body(c, fn, index_live_budget, 0, array) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value = class_value(fn);
        if (!(fvm_object_set(c, number, "valueOf", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = class_value(number);
        if (!(fvm_object_set(c, array, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = 64;
        fvm_value_number(&value, 81);
        if (!(fvm_object_set(c, array, "2147483647", &value) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = cfg.instruction_limit;
        int own = 1;
        if (!(fvm_object_has_own(c, array, "2147483647", &own) == FVM_OK && !own && !c->avm1_execution_depth &&
              !c->gc_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, array, "2147483647", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
