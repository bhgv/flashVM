/* A deleted length skips element getters and retains the detached list through target GC. */
#define FVM_APPLY_DELETED_LENGTH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_apply_deleted_length.cpp"

static unsigned deleted_index_calls, deleted_target_calls, deleted_fault;


static FVM_Result deleted_index(FVM_Context *, const FVM_ScriptFrame *, FVM_Value *, void *)
{
    ++deleted_index_calls;
    return FVM_ERROR;
}


static FVM_Result deleted_target(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++deleted_target_calls;
    if (!(frame->argc == 0 && frame->capture))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = 0;
    if (!(fvm_object_delete(c, c->global, "deletedLengthList", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, c->global, "deletedLengthFunction", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool alive = false;
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        alive |= c->objects[i] == frame->capture;
    }
    if (!(alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 91);
    return deleted_fault == 1 ? FVM_ERROR : deleted_fault == 2 ? fvm_exception_raise(c, out) : FVM_OK;
}


static void deleted_index_getter(FVM_Context *c, FVM_Object *array)
{
    FVM_Object *fn = deleted_length_function(c);
    if (!(fvm_function_set_body(c, fn, deleted_index, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value args[3], out;
    fvm_value_string(args, "0");
    args[1] = class_value(fn);
    fvm_value_undefined(args + 2);
    args[2].type = FVM_VALUE_NULL;
    if (!(fvm_object_call(c, array, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_APPLY_DELETED_ROOTS_FIXTURE_ONLY


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
        for (deleted_fault = 0; deleted_fault < 3; ++deleted_fault)
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
                FVM_Object *array = deleted_length_array(c), *fn = deleted_length_function(c);
                deleted_index_getter(c, array);
                if (!(fvm_function_set_body(c, fn, deleted_target, 0, array) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value value = class_value(fn), out;
                if (!(fvm_set_global(c, "deletedLengthFunction", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                value = class_value(array);
                if (!(fvm_set_global(c, "deletedLengthList", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                const FVM_Result expected = deleted_fault == 1   ? FVM_ERROR
                                            : deleted_fault == 2 ? FVM_EXCEPTION
                                                                 : FVM_OK;
                deleted_index_calls = deleted_target_calls = 0;
                if (!route)
                {
                    FVM_Value args[2];
                    fvm_value_undefined(args);
                    args[0].type = FVM_VALUE_NULL;
                    args[1] = value;
                    if (!(fvm_object_call(c, fn, "apply", 2, args, &out) == expected))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (expected == FVM_EXCEPTION)
                    {
                        if (!(fvm_exception_clear(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                else
                {
                    Program p;
                    p.variable("deletedLengthList");
                    p.op(0x96);
                    p.op(1);
                    p.op(0);
                    p.op(2);
                    p.number(2);
                    p.variable("deletedLengthFunction");
                    p.text("apply");
                    p.op(0x52);
                    p.finish();
                    out = boxing_run(c, p, route - 1, expected);
                }
                if (!(deleted_target_calls == 1 && !deleted_index_calls && !c->exception_pending))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (expected == FVM_OK)
                {
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 91))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->script_this_cell))
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
#endif
