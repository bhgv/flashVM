/* Coercible recreated length can remove its own binding and collect during Array methods. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/standard/base_state.hpp"

#define FVM_ARRAY_DELETED_LENGTH_FIXTURE_ONLY
#include "array_deleted_length.cpp"

static unsigned deleted_array_conversions, deleted_array_fault;


static FVM_Result deleted_array_number(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++deleted_array_conversions;
    if (!(frame->capture))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int removed = 0;
    if (!(fvm_object_delete(c, frame->capture, "length", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, c->global, "arrayDeletedTarget", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == (c->gc_native_depth ? FVM_INVALID_ARGUMENT : FVM_OK)))
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
    fvm_value_number(out, 1);
    return deleted_array_fault == 1 ? FVM_ERROR : deleted_array_fault == 2 ? fvm_exception_raise(c, out) : FVM_OK;
}


/* Raw fixture storage defers the destructive callback until the operation under test. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *methods[] = {"join", "slice", "push", "unshift", "sort"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned method = 0; method < 5; ++method)
        {
            for (deleted_array_fault = 0; deleted_array_fault < 3; ++deleted_array_fault)
            {
                for (unsigned route = 0; route < 5; ++route)
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
                    FVM_Object *array = deleted_length_array(c), *number = 0, *fn = deleted_length_function(c);
                    if (!(fvm_construct(c, "Object", 0, 0, &number) == FVM_OK &&
                          fvm_function_set_body(c, fn, deleted_array_number, 0, array) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value value = class_value(fn), out;
                    if (!(fvm_object_set(c, number, "valueOf", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = class_value(number);
                    if (!(fvm_base_store_data(c, array, "length", value, false) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = class_value(array);
                    if (!(fvm_set_global(c, "arrayDeletedTarget", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    const FVM_Result expected = !deleted_array_fault       ? FVM_OK
                                                : deleted_array_fault == 1 ? FVM_ERROR
                                                                           : FVM_EXCEPTION;
                    const unsigned argc = method == 2 || method == 3 ? 1 : 0;
                    deleted_array_conversions = 0;
                    if (route == 4)
                    {
                        if (!(fvm_gc_pin(c, array) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Object *owner = array;
                        FVM_NativeEntry entry;
                        bool found = false;
                        for (; owner && !found; owner = owner->prototype)
                        {
                            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method_entry =
                                owner->methods.find(methods[method]);
                            if (method_entry != owner->methods.end())
                            {
                                entry = method_entry->second;
                                found = true;
                            }
                        }
                        if (!(found))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_value_number(&value, 81);
                        if (!(entry.callback(c, array, argc, &value, &out, entry.userdata) == expected))
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
                    else if (!route)
                    {
                        fvm_value_number(&value, 81);
                        if (!(fvm_object_call(c, array, methods[method], argc, &value, &out) == expected))
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
                        if (argc)
                        {
                            p.number(81);
                        }
                        p.number(argc);
                        p.variable("arrayDeletedTarget");
                        p.text(methods[method]);
                        p.op(0x52);
                        p.finish();
                        out = boxing_run(c, p, route - 1, expected);
                    }
                    if (!(deleted_array_conversions == 1 && !c->exception_pending))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!deleted_array_fault)
                    {
                        if (!method)
                        {
                            if (!(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "11")))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (method == 1)
                        {
                            if (!(out.type == FVM_VALUE_OBJECT &&
                                  fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == 1))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (argc)
                        {
                            if (!(out.type == FVM_VALUE_NUMBER && out.number == 2))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    if (!(fvm_gc_pin(c, array) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    int own = 0;
                    if (!(fvm_object_has_own(c, array, "length", &own) == FVM_OK &&
                          !!own == (!deleted_array_fault && method >= 2)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
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
    }
    fvm_runtime_destroy(rt);
}
