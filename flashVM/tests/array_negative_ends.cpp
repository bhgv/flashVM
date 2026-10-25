/* Signed borrowed end operations retain their distinct length and deletion rules. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"


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
        FVM_Object *array = 0, *o = 0;
        FVM_Value value, args[4], out;
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
        fvm_value_number(&value, -3);
        if (!(fvm_object_set(c, o, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 4; ++i)
        {
            fvm_value_number(args + i, i + 10);
        }
        if (!(borrow(c, array, "push", o, 4, args, &out) == FVM_OK && out.number == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "length", 1);
        number(c, o, "-3", 10);
        number(c, o, "-2", 11);
        number(c, o, "-1", 12);
        number(c, o, "0", 13);
        fvm_value_number(&value, -2);
        if (!(fvm_object_set(c, o, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(borrow(c, array, "pop", o, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 10))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int own = 1;
        if (!(fvm_object_has_own(c, o, "-3", &own) == FVM_OK && !own))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "length", -2);
        if (!(borrow(c, array, "shift", o, 0, 0, &out) == FVM_OK && out.number == 13))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "0", 13);
        number(c, o, "length", -2);
        if (!(borrow(c, array, "unshift", o, 4, args, &out) == FVM_OK && out.number == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "0", 10);
        number(c, o, "3", 13);
        number(c, o, "length", -2);
        number(c, o, "-2", 11);
        if (!(borrow(c, array, "push", o, 0, 0, &out) == FVM_OK && out.number == -2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_property_flags(c, o, "length", 4, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_property_flags(c, o, "-2", 4, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(borrow(c, array, "push", o, 1, args, &out) == FVM_OK && out.number == -1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "length", -2);
        number(c, o, "-2", 11);
        if (!(fvm_object_set_property_flags(c, o, "length", 0, 4) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 55);
        if (!(fvm_object_set(c, o, "-3", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_property_flags(c, o, "-3", 2, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(borrow(c, array, "pop", o, 0, 0, &out) == FVM_OK && out.number == 55))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "-3", 55);
        number(c, o, "length", -2);
        if (!(fvm_object_set_property_flags(c, o, "-3", 0, 2) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int removed = 0;
        if (!(fvm_object_delete(c, o, "-3", &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_accessor_rooted(c, o, "-3", getter, 0, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        reads = 0;
        if (!(borrow(c, array, "pop", o, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED && reads == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, -2147483648.0);
        if (!(fvm_object_set(c, o, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(borrow(c, array, "pop", o, 0, 0, &out) == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(borrow(c, array, "shift", o, 0, 0, &out) == FVM_OK && out.number == 10))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(borrow(c, array, "push", o, 1, args, &out) == FVM_OK && out.number == -2147483647.0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "-2147483648", 10);
        fvm_value_number(&value, -2);
        if (!(fvm_object_set(c, o, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned long old_limit = rt->config.instruction_limit;
        rt->config.instruction_limit = 1;
        if (!(borrow(c, array, "push", o, 4, args, &out) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = old_limit;
        number(c, o, "length", -2);
        fvm_value_number(&value, 4294967295.0);
        if (!(fvm_object_set(c, array, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const char *methods[] = {"push", "pop", "shift", "unshift"};
        const double lengths[] = {-1, -2, -3, -3};
        for (unsigned i = 0; i < 4; ++i)
        {
            if (!(borrow(c, array, methods[i], array, 0, 0, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (i == 0 || i == 3)
            {
                if (!(out.type == FVM_VALUE_NUMBER && out.number == lengths[i]))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(out.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            number(c, array, "length", lengths[i]);
        }
        if (!(c->gc_frames == 0 && c->avm1_execution_depth == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
