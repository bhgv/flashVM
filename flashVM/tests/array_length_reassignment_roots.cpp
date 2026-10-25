/* Coercion reselects live length descriptors and keeps detached inputs alive without double calls. */
#define FVM_REASSIGNMENT_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_length_reassignment.cpp"

static FVM_Object *reassign_root_array, *reassign_root_input, *reassign_root_meta;
static FVM_AccessorEntry reassign_root_native;
static unsigned reassign_root_mode, reassign_root_fault, reassign_root_calls;


static bool reassign_root_alive(FVM_Context *c, FVM_Object *o)
{
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        if (c->objects[i] == o)
        {
            return true;
        }
    }
    return false;
}


static FVM_Result reassign_root_convert(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++reassign_root_calls;
    int removed = 0;
    if (!(fvm_object_delete(c, c->global, "reassignmentInput", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, c->global, "reassignmentTarget", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!reassign_root_mode)
    {
        FVM_Value n;
        fvm_value_number(&n, 2);
        if (!(fvm_object_set(c, reassign_root_array, "length", &n) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(fvm_object_delete(c, reassign_root_array, "length", &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (reassign_root_mode == 2)
        {
            reassign_root_array->accessors["length"] = reassign_root_native;
            fvm_value_number(&reassign_root_array->resource_result, 3);
        }
    }
    if (!(fvm_gc_collect(c, 0) == (c->gc_native_depth ? FVM_INVALID_ARGUMENT : FVM_OK)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(reassign_root_alive(c, reassign_root_array) && reassign_root_alive(c, reassign_root_input) &&
          reassign_root_alive(c, reassign_root_meta)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 1);
    return reassign_root_fault == 1 ? FVM_ERROR : reassign_root_fault == 2 ? fvm_exception_raise(c, out) : FVM_OK;
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
        for (unsigned route = 0; route < 4; ++route)
        {
            for (reassign_root_mode = 0; reassign_root_mode < 3; ++reassign_root_mode)
            {
                for (reassign_root_fault = 0; reassign_root_fault < 3; ++reassign_root_fault)
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
                    if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *native = 0;
                    if (!(fvm_construct(c, "Array", 0, 0, &native) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    reassign_root_native = native->accessors.find("length")->second;
                    reassign_root_array = reassignment_array(c, 1);
                    if (!(fvm_construct(c, "Object", 0, 0, &reassign_root_input) == FVM_OK &&
                          fvm_object_create(c, &reassign_root_meta) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value input = class_value(reassign_root_input), edge = class_value(reassign_root_meta), out;
                    if (!(fvm_object_set(c, reassign_root_input, "edge", &edge) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *fn = version_function(c, 1);
                    if (!(fvm_function_set_body(c, fn, reassign_root_convert, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    edge = class_value(fn);
                    if (!(fvm_object_set(c, reassign_root_input, "valueOf", &edge) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    reassign_root_calls = 0;
                    const FVM_Result expected = !reassign_root_fault       ? FVM_OK
                                                : reassign_root_fault == 1 ? FVM_ERROR
                                                                           : FVM_EXCEPTION;
                    if (!(reassignment_write(c, reassign_root_array, input, route, expected) == expected &&
                          reassign_root_calls == 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (c->exception_pending)
                    {
                        if (!(fvm_exception_clear(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_gc_pin(c, reassign_root_array) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_get(c, reassign_root_array, "length", &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!reassign_root_fault)
                    {
                        if (!(out.type == FVM_VALUE_OBJECT && out.object == reassign_root_input &&
                              reassign_root_alive(c, reassign_root_meta)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (reassign_root_mode == 1)
                    {
                        if (!(out.type == FVM_VALUE_UNDEFINED))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == (reassign_root_mode == 2 ? 3 : 2)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    int own = 0;
                    if (!(fvm_object_has_own(c, reassign_root_array, "1", &own) == FVM_OK &&
                          !!own == (!!reassign_root_fault || reassign_root_mode == 1)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
