/* A length Function detached by its own valueOf remains live through Array coercion. */
#define FVM_METHOD_LENGTH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_method_length.cpp"

static FVM_Object *method_length_receiver, *method_length_function;
static unsigned method_length_calls, method_length_fault;


static FVM_Result method_length_conversion(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++method_length_calls;
    int removed = 0;
    if (!(fvm_object_delete(c, method_length_receiver, "length", &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_delete(c, c->global, "arrayDeletedTarget", &removed) == FVM_OK &&
          fvm_object_delete(c, c->global, "indexLiveTarget", &removed) == FVM_OK))
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
        alive |= c->objects[i] == method_length_function;
    }
    if (!(alive))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 1);
    return method_length_fault == 1 ? FVM_ERROR : method_length_fault == 2 ? fvm_exception_raise(c, out) : FVM_OK;
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
        for (unsigned method = 0; method < 3; ++method)
        {
            for (method_length_fault = 0; method_length_fault < 3; ++method_length_fault)
            {
                for (unsigned route = 0; route < 5; ++route)
                {
                    if ((route == 2 || route == 3) && !fvm_jit_available())
                    {
                        continue;
                    }
                    if (route == 4 && method == 2)
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
                    method_length_receiver = method_length_array(c, 0, true, &method_length_function);
                    FVM_Object *fn = version_function(c, 1);
                    if (!(fvm_function_set_body(c, fn, method_length_conversion, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value value = class_value(fn), out;
                    if (!(fvm_object_set(c, method_length_function, "valueOf", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    value = class_value(method_length_receiver);
                    if (!(fvm_set_global(c, "arrayDeletedTarget", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Result expected = !method_length_fault       ? FVM_OK
                                          : method_length_fault == 1 ? FVM_ERROR
                                                                     : FVM_EXCEPTION;
                    method_length_calls = 0;
                    const char *name = method ? "push" : "join";
                    if (method == 2)
                    {
                        fvm_value_number(&value, 81);
                        if (!(index_live_write(c, method_length_receiver, "3", value, route, expected) == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (route == 4)
                    {
                        if (!(fvm_gc_pin(c, method_length_receiver) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Object *owner = method_length_receiver;
                        FVM_NativeEntry entry;
                        bool found = false;
                        for (; owner && !found; owner = owner->prototype)
                        {
                            fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator it =
                                owner->methods.find(name);
                            if (it != owner->methods.end())
                            {
                                entry = it->second;
                                found = true;
                            }
                        }
                        if (!(found))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_value_number(&value, 81);
                        if (!(entry.callback(c, method_length_receiver, method ? 1 : 0, &value, &out, entry.userdata) ==
                              expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if (!route)
                    {
                        fvm_value_number(&value, 81);
                        if (!(fvm_object_call(c, method_length_receiver, name, method ? 1 : 0, &value, &out) ==
                              expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        Program p;
                        if (method)
                        {
                            p.number(81);
                        }
                        p.number(method ? 1 : 0);
                        p.variable("arrayDeletedTarget");
                        p.text(name);
                        p.op(0x52);
                        p.finish();
                        out = boxing_run(c, p, route - 1, expected);
                    }
                    if (c->exception_pending)
                    {
                        if (!(fvm_exception_clear(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(method_length_calls == 1 && fvm_gc_pin(c, method_length_receiver) == FVM_OK &&
                          fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!method_length_fault && method)
                    {
                        if (!(fvm_object_get(c, method_length_receiver, "length", &value) == FVM_OK &&
                              value.type == FVM_VALUE_NUMBER && value.number == (method == 2 ? 4 : 2)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!method_length_fault && !method)
                    {
                        if (!(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "11")))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
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
