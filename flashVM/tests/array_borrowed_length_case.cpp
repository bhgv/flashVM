/* Borrowed loaded Array methods resolve raw own LENGTH without executing its callbacks. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include "../src/standard/base_state.hpp"

static unsigned callback_calls;


static FVM_Result trap(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++callback_calls;
    fvm_value_number(out, 99);
    return FVM_OK;
}
#ifdef FVM_ARRAY_BORROWED_CASE_JIT
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"


static FVM_Result native_borrow(FVM_Context *c,
                                FVM_Object *array,
                                const char *method,
                                FVM_Object *receiver,
                                unsigned n,
                                const FVM_Value *args,
                                FVM_Value *out)
{
    FVM_Value value;
    if (!(fvm_object_get(c, array, method, &value) == FVM_OK && fvm_set_global(c, "borrowFunction", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = object(receiver);
    if (!(fvm_set_global(c, "borrowReceiver", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Program call;
    for (unsigned i = n; i; --i)
    {
        if (!(args[i - 1].type == FVM_VALUE_NUMBER))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        call.number(args[i - 1].number);
    }
    call.variable("borrowReceiver");
    call.number(n + 1);
    call.variable("borrowFunction");
    call.text("call");
    call.op(0x52);
    call.finish();
    *out = class_run(c, call);
    return FVM_OK;
}

#define borrow native_borrow
#endif


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#ifdef FVM_ARRAY_BORROWED_CASE_JIT
    if (!fvm_jit_available())
    {
        fvm_runtime_destroy(rt);
        return 0;
    }
    for (class_mode = 0; class_mode < 3; ++class_mode)
#endif
        for (unsigned version = 4; version <= 8; ++version)
        {
            for (unsigned kind = 0; kind < 3; ++kind)
            {
                for (unsigned method = 0; method < 3; ++method)
                {
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                    swf[3] = (unsigned char)version;
                    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *array = 0, *o = 0, *fn = 0;
                    FVM_Value args[3], value, out;
                    if (!(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK && fvm_gc_pin(c, o) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_number(&value, 2);
                    if (!(fvm_object_set(c, o, "LENGTH", &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_number(&value, 10);
                    if (!(fvm_table_set_index(c, o, 0, &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_number(&value, 20);
                    if (!(fvm_table_set_index(c, o, 1, &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (kind)
                    {
                        if (!(fvm_function_create_native(c, trap, 0, 0, &fn) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_value_string(args, "LENGTH");
                        args[1] = args[2] = object(fn);
                        if (!(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (kind == 2)
                        {
                            if (!(fvm_object_set_property_flags(c, o, "LENGTH", FVM_PROPERTY_READ_ONLY, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                    fvm_value_number(&value, 90);
                    if (!(fvm_object_set_bytes(c, o, "LENGTH\0tail", 11, &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    callback_calls = 0;
                    bool folded = version <= 6;
                    if (method == 0)
                    {
                        if (!(borrow(c, array, "slice", o, 0, 0, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        number(c, out.object, "length", folded ? 2 : 0);
                        if (folded)
                        {
                            number(c, out.object, "0", 10);
                            number(c, out.object, "1", 20);
                        }
                    }
                    else if (method == 1)
                    {
                        fvm_value_number(args, 30);
                        if (!(borrow(c, array, "push", o, 1, args, &out) == FVM_OK && out.number == (folded ? 3 : 1)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        number(c, o, folded ? "2" : "0", 30);
                    }
                    else
                    {
                        fvm_value_number(args, 0);
                        fvm_value_number(args + 1, 1);
                        if (!(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        number(c, out.object, "length", folded ? 1 : 0);
                        if (folded)
                        {
                            number(c, out.object, "0", 10);
                            number(c, o, "0", 20);
                        }
                    }
                    if (!(callback_calls == 0))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value stored;
                    if (!kind)
                    {
                        stored = o->properties[fvm_detail::string("LENGTH")];
                    }
                    else
                    {
                        if (!(fvm_base_accessor_data(o->accessors.find(fvm_detail::string("LENGTH"))->second, stored)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(stored.type == FVM_VALUE_NUMBER &&
                          stored.number == (folded && kind != 2 && method ? (method == 1 ? 3 : 1) : 2)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (folded)
                    {
                        if (!(o->properties.find(fvm_detail::string("length")) == o->properties.end()))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_object_get_bytes(c, o, "LENGTH\0tail", 11, &value) == FVM_OK && value.number == 90))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
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
