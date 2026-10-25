/* Ordinary indexed member calls retain owner/receiver/capture across detachment and errors. */
#define FVM_SUPER_WRITE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "super_property_write.cpp"
#include "flashvm/exception.h"

static FVM_Object *ordinary_indexed_owner, *ordinary_indexed_receiver, *ordinary_indexed_capture,
    *ordinary_indexed_function;
static unsigned ordinary_indexed_mode, ordinary_indexed_calls;


static FVM_Result ordinary_indexed_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++ordinary_indexed_calls;
    if (!(frame->receiver == ordinary_indexed_receiver && c->calling_owner == ordinary_indexed_owner &&
          c->calling_function == ordinary_indexed_function))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (ordinary_indexed_mode == 2)
    {
        int removed = 0;
        if (!(fvm_object_delete(c, ordinary_indexed_owner, "5", &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_prototype(c, ordinary_indexed_receiver, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool owner_alive = false, function_alive = false, capture_alive = false;
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        owner_alive |= c->objects[i] == ordinary_indexed_owner;
        function_alive |= c->objects[i] == ordinary_indexed_function;
        capture_alive |= c->objects[i] == ordinary_indexed_capture;
    }
    if (!(owner_alive && function_alive && capture_alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, ordinary_indexed_capture, "marker", out) == FVM_OK && out->number == 81))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (ordinary_indexed_mode == 1)
    {
        return fvm_exception_raise(c, out);
    }
    return ordinary_indexed_mode == 3 ? FVM_ERROR : FVM_OK;
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
        for (unsigned array = 0; array < 2; ++array)
        {
            for (ordinary_indexed_mode = 0; ordinary_indexed_mode < 4; ++ordinary_indexed_mode)
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
                    if (!(fvm_construct(c, "Object", 0, 0, &ordinary_indexed_receiver) == FVM_OK &&
                          fvm_gc_pin(c, ordinary_indexed_receiver) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_construct(c, array ? "Array" : "Object", 0, 0, &ordinary_indexed_owner) == FVM_OK &&
                          fvm_construct(c, "Object", 0, 0, &ordinary_indexed_capture) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value value, out;
                    fvm_value_number(&value, 81);
                    if (!(fvm_object_set(c, ordinary_indexed_capture, "marker", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    const unsigned char end[] = {0};
                    if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &ordinary_indexed_function) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_function_set_body(
                              c, ordinary_indexed_function, ordinary_indexed_body, 0, ordinary_indexed_capture) ==
                          FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = class_value(ordinary_indexed_function);
                    if (!(fvm_table_set_index(c, ordinary_indexed_owner, 5, &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_set_prototype(c, ordinary_indexed_owner, ordinary_indexed_receiver->prototype) ==
                              FVM_OK &&
                          fvm_object_set_prototype(c, ordinary_indexed_receiver, ordinary_indexed_owner) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = class_value(ordinary_indexed_receiver);
                    if (!(fvm_set_global(c, "ordinaryIndexedTarget", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    ordinary_indexed_calls = 0;
                    const FVM_Result expected = ordinary_indexed_mode == 1   ? FVM_EXCEPTION
                                                : ordinary_indexed_mode == 3 ? FVM_ERROR
                                                                             : FVM_OK;
                    if (!route)
                    {
                        if (!(fvm_object_call(c, ordinary_indexed_receiver, "5", 0, 0, &out) == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (expected == FVM_EXCEPTION)
                        {
                            if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == 81))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_exception_clear(c) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    else
                    {
                        Program code;
                        code.number(0);
                        code.variable("ordinaryIndexedTarget");
                        code.text("5");
                        code.op(0x52);
                        code.finish();
                        out = boxing_run(c, code, route - 1, expected);
                    }
                    if (!(ordinary_indexed_calls == 1))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (expected == FVM_OK)
                    {
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == 81))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function &&
                          !c->exception_pending))
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
