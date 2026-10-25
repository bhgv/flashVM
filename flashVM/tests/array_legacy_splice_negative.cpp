/* Borrowed negative splice uses signed raw keys and captures length before deletion conversion. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"

static unsigned conversions;


static FVM_Result deletion(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *data)
{
    FVM_Value value;
    ++conversions;
    fvm_value_number(&value, 99);
    if (!(fvm_object_set(c, (FVM_Object *)data, "length", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 1);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
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
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *array = 0, *o = 0, *counter = 0;
        FVM_Value value, args[6], out;
        if (!(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &counter) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, counter, "valueOf", deletion, o, o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 4294967293.0);
        if (!(fvm_object_set(c, o, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 77);
        if (!(fvm_object_set(c, o, "10", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, 100);
        args[1] = object(counter);
        for (unsigned i = 2; i < 6; ++i)
        {
            fvm_value_number(args + i, i + 10);
        }
        conversions = 0;
        if (!(borrow(c, array, "splice", o, 6, args, &out) == FVM_OK && conversions == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, out.object, "length", 0);
        number(c, o, "length", 1);
        number(c, o, "-3", 12);
        number(c, o, "-2", 13);
        number(c, o, "-1", 14);
        number(c, o, "0", 15);
        number(c, o, "10", 77);
        fvm_value_number(&value, -3);
        if (!(fvm_object_set(c, o, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, -1);
        fvm_value_number(args + 1, 1);
        if (!(borrow(c, array, "splice", o, 6, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "length", -3);
        number(c, o, "-3", 12);
        fvm_value_number(args, 0);
        fvm_value_number(args + 1, -1);
        if (!(borrow(c, array, "splice", o, 6, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(args + 1);
        if (!(borrow(c, array, "splice", o, 6, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(borrow(c, array, "splice", o, 1, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, out.object, "length", 0);
        number(c, o, "length", -3);
        fvm_value_number(args, -2147483648.0);
        if (!(borrow(c, array, "splice", o, 1, args, &out) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, 0);
        fvm_value_number(args + 1, 0);
        fvm_value_number(&value, -3);
        if (!(fvm_object_set(c, o, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_property_flags(c, o, "-3", 4, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_property_flags(c, o, "length", 4, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args + 2, 123);
        if (!(borrow(c, array, "splice", o, 3, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "-3", 12);
        number(c, o, "length", -3);
        if (!(fvm_object_set_property_flags(c, o, "length", 0, 4) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned long old_limit = rt->config.instruction_limit;
        rt->config.instruction_limit = 1;
        if (!(borrow(c, array, "splice", o, 6, args, &out) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = old_limit;
        number(c, o, "length", -3);
        fvm_value_number(&value, 4294967295.0);
        if (!(fvm_object_set(c, array, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        args[1] = object(counter);
        conversions = 0;
        if (!(borrow(c, array, "splice", array, 3, args, &out) == FVM_OK && conversions == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, out.object, "length", 0);
        number(c, array, "length", 0);
        number(c, array, "-1", 123);
        number(c, o, "length", 99);
        if (!(c->gc_frames == 0 && c->avm1_execution_depth == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
