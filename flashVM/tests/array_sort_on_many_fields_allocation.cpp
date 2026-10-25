/* Large field preparation propagates OOM and budgets charge both preparation and comparison. */
#define main many_fields_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define FVM_MANY_FIELDS_FIXTURE_ONLY
#include "array_sort_on_many_fields.cpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; version += 4)
    {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 2048 && !success; ++boundary)
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
            FVM_Object *receiver = many_fields_receiver(c), *fields = many_fields_list(c, 65, false), *original[3];
            FVM_Value value, args[2] = {class_value(fields), class_value(fields)}, out;
            fvm_value_number(args + 1, 16);
            for (unsigned i = 0; i < 3; ++i)
            {
                if (!(fvm_table_get_index(c, receiver, i, &value) == FVM_OK && fvm_gc_pin(c, value.object) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                original[i] = value.object;
            }
            allowance = boundary;
            const FVM_Result status = fvm_object_call(c, receiver, "sortOn", 2, args, &out);
            allowance = -1;
            if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (status == FVM_OUT_OF_MEMORY)
            {
                failed = true;
                /* Error paths need not roll back: restore pinned input before retry and GC. */
                for (unsigned i = 0; i < 3; ++i)
                {
                    value = class_value(original[i]);
                    if (!(fvm_table_set_index(c, receiver, i, &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK &&
                      fvm_object_call(c, receiver, "sortOn", 2, args, &out) == FVM_OK))
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
            if (!(out.type == FVM_VALUE_OBJECT && out.object == receiver))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned i = 0; i < 3; ++i)
            {
                FVM_Value item;
                if (!(fvm_table_get_index(c, receiver, i, &item) == FVM_OK &&
                      fvm_object_get(c, item.object, "key", &value) == FVM_OK && value.number == i + 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_context_destroy(c);
        }
        if (!(failed && success))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    const unsigned limits[] = {32, 100};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned limit = 0; limit < 2; ++limit)
        {
            for (unsigned route = 0; route < 4; ++route)
            {
                if (route >= 2 && !fvm_jit_available())
                {
                    continue;
                }
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
                FVM_Object *receiver = many_fields_receiver(c), *fields = many_fields_list(c, 65, false);
                FVM_Value options, out, value;
                fvm_value_number(&options, 16);
                const unsigned long saved = rt->config.instruction_limit;
                rt->config.instruction_limit = limits[limit];
                out = many_fields_call(c, receiver, fields, options, route, FVM_ERROR);
                rt->config.instruction_limit = saved;
                const unsigned original[] = {2, 1, 3};
                for (unsigned i = 0; i < 3; ++i)
                {
                    FVM_Value item;
                    if (!(fvm_table_get_index(c, receiver, i, &item) == FVM_OK &&
                          fvm_object_get(c, item.object, "key", &value) == FVM_OK && value.number == original[i]))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                out = many_fields_call(c, receiver, fields, options, route);
                if (!(out.type == FVM_VALUE_OBJECT && out.object == receiver))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (unsigned i = 0; i < 3; ++i)
                {
                    FVM_Value item;
                    if (!(fvm_table_get_index(c, receiver, i, &item) == FVM_OK &&
                          fvm_object_get(c, item.object, "key", &value) == FVM_OK && value.number == i + 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->avm1_execution_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
