#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/transfer.h"
#include <cstring>

static unsigned int reads;


static FVM_Result getter(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{
    ++reads;
    fvm_value_number(out, 4);
    return FVM_OK;
}


static FVM_Value object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = o;
    return v;
}


static FVM_Result borrow(FVM_Context *c,
                         FVM_Object *array,
                         const char *name,
                         FVM_Object *receiver,
                         unsigned int n,
                         const FVM_Value *args,
                         FVM_Value *out)
{
    FVM_Value method, call[8];
    if (!(n < 7))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, array, name, &method) == FVM_OK && method.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call[0] = object(receiver);
    for (unsigned int i = 0; i < n; ++i)
    {
        call[i + 1] = args[i];
    }
    return fvm_object_call(c, method.object, "call", n + 1, call, out);
}


static void number(FVM_Context *c, FVM_Object *o, const char *name, double expected)
{
    FVM_Value v;
    if (!(fvm_object_get(c, o, name, &v) == FVM_OK && v.type == FVM_VALUE_NUMBER && v.number == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}

static FVM_Object *source, *new_proto;
static unsigned starts, order;
static bool inherited_mode;
static unsigned setters;


static FVM_Result reject_destination(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *)
{
    ++setters;
    return FVM_OUT_OF_MEMORY;
}


static FVM_Result reject_start(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *, void *)
{
    order = order * 10 + 9;
    return FVM_OUT_OF_MEMORY;
}


static FVM_Result convert_start(FVM_Context *c, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{
    ++starts;
    order = order * 10 + 1;
    if (inherited_mode)
    {
        if (!(fvm_object_define_accessor_rooted(c, new_proto, "1", getter, reject_destination, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(out, 0);
    }
    else
    {
        FVM_Value v;
        fvm_value_number(&v, 44);
        if (!(fvm_table_set_index(c, source, 4, &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(out, 1);
    }
    return FVM_OK;
}


static FVM_Result read_length(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{
    order = order * 10 + 2;
    fvm_value_number(out, 5);
    return FVM_OK;
}

static unsigned count_calls;
static bool shrink_count;
static bool install_count_getter = true;


static FVM_Result convert_count(FVM_Context *c, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *out, void *)
{
    ++count_calls;
    if (install_count_getter)
    {
        if (!(fvm_object_define_accessor_rooted(c, new_proto, "1", getter, reject_destination, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Value v;
    fvm_value_number(&v, 44);
    if (!(fvm_table_set_index(c, source, 4, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (shrink_count)
    {
        fvm_value_number(&v, 2);
        if (!(fvm_object_set(c, source, "length", &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_number(out, 2);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *array = 0, *start = 0, *fn = 0;
    FVM_Value v, out, args[2];
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &start) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_native(c, convert_start, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v = object(fn);
    if (!(fvm_object_set(c, start, "valueOf", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    args[0] = object(start);
    for (unsigned i = 0; i < 2; ++i)
    {
        fvm_value_number(&v, 3);
        if (!(fvm_construct(c, i ? "Object" : "Array", i ? 0 : 1, i ? 0 : &v, &source) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (i)
        {
            if (!(fvm_object_define_accessor_rooted(c, source, "length", read_length, 0, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        starts = order = 0;
        if (!(borrow(c, array, "splice", source, 1, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(starts == 1 && order == (i ? 12U : 1U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, out.object, "length", 4);
        number(c, out.object, "3", 44);
    }
    inherited_mode = true;
    fvm_value_number(&v, 3);
    if (!(fvm_construct(c, "Array", 1, &v, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &new_proto) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set_prototype(c, source, new_proto) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    starts = reads = 0;
    if (!(borrow(c, array, "splice", source, 1, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(starts == 1 && reads == 1 && setters == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int own = 0;
    if (!(fvm_object_has_own(c, out.object, "1", &own) == FVM_OK && own))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    number(c, out.object, "1", 4);
    number(c, source, "length", 0);
    if (!(fvm_construct(c, "Object", 0, 0, &source) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_accessor_rooted(c, source, "length", read_length, 0, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_native(c, reject_start, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v = object(fn);
    if (!(fvm_object_set(c, start, "valueOf", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    order = 0;
    if (!(borrow(c, array, "splice", source, 1, args, &out) == FVM_OUT_OF_MEMORY && order == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *count = 0;
    if (!(fvm_construct(c, "Object", 0, 0, &count) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_native(c, convert_count, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    v = object(fn);
    if (!(fvm_object_set(c, count, "valueOf", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 0);
    args[1] = object(count);
    for (unsigned i = 0; i < 3; ++i)
    {
        shrink_count = i == 1;
        install_count_getter = i != 2;
        fvm_value_number(&v, 3);
        if (!(fvm_construct(c, "Array", 1, &v, &source) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &new_proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_prototype(c, source, new_proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 7);
        if (!(fvm_table_set_index(c, source, 0, &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&v, 9);
        if (!(fvm_table_set_index(c, source, 2, &v) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        reads = setters = count_calls = 0;
        if (!(borrow(c, array, "splice", source, 2, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(count_calls == 1 && reads == (i == 2 ? 0U : 1U) && setters == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, out.object, "length", 2);
        number(c, out.object, "0", 7);
        if (i != 2)
        {
            number(c, out.object, "1", 4);
        }
        else
        {
            if (!(fvm_object_has_own(c, out.object, "1", &own) == FVM_OK && !own))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        number(c, source, "length", 1);
        if (!(fvm_object_has_own(c, source, "2", &own) == FVM_OK && !own))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (i != 1)
        {
            number(c, source, "0", 9);
        }
        else
        {
            if (!(fvm_object_has_own(c, source, "0", &own) == FVM_OK && !own))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
