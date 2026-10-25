/* Loaded generic splice uses own presence/raw data and leaves borrowed tails beyond new length. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"

static unsigned writes, watched;


static FVM_Result setter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++writes;
    fvm_value_undefined(out);
    return FVM_OK;
}


static FVM_Result watcher(FVM_Context *, FVM_Object *, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    if (!(n == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++watched;
    *out = a[2];
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
    for (unsigned version = 6; version <= 8; ++version)
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
        if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *array = 0, *get_fn = 0, *set_fn = 0, *watch_fn = 0, *proto = 0;
        if (!(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, array) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_native(c, getter, 0, 0, &get_fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, get_fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_native(c, setter, 0, 0, &set_fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, set_fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_native(c, watcher, 0, 0, &watch_fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, watch_fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value, args[3], out;
        fvm_value_number(&value, 77);
        if (!(fvm_object_set(c, proto, "2", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned mode = 0; mode < 3; ++mode)
        {
            FVM_Object *o = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(c, o) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_set_prototype(c, o, proto) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 4);
            if (!(fvm_object_set(c, o, "length", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned i = 0; i < 4; ++i)
            {
                if (mode == 2 && i == 2)
                {
                    continue;
                }
                fvm_value_number(&value, 9 - i);
                if (!(fvm_table_set_index(c, o, i, &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            args[1] = object(get_fn);
            args[2] = object(set_fn);
            fvm_value_string(args, "length");
            if (!(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(args, "1");
            if (!(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (mode != 2)
            {
                fvm_value_string(args, "2");
                if (!(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_set_property_flags(c, o, "2", 2, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (mode == 1)
            {
                if (!(fvm_object_set_property_flags(c, o, "1", 6, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_value_string(args, "1");
            args[1] = object(watch_fn);
            if (!(fvm_object_call(c, o, "watch", 2, args, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(args, 1);
            fvm_value_number(args + 1, 1);
            reads = writes = watched = 0;
            if (!(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK && reads == 0 && writes == 0 && watched == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, out.object, "0", 8);
            number(c, out.object, "length", 1);
            int own = 0;
            if (!(fvm_object_has_own(c, o, "3", &own) == FVM_OK && own))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(borrow(c, array, "slice", o, 0, 0, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, out.object, "length", 3);
            if (!(fvm_table_get_index(c, out.object, 1, &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (mode == 2)
            {
                if (!(value.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(value.type == FVM_VALUE_NUMBER && value.number == (mode == 1 ? 8 : 7)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            number(c, out.object, "2", 6);
        }
        FVM_Object *o = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 1);
        if (!(fvm_object_set(c, o, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string_bytes(&value, "a\0b", 3);
        if (!(fvm_object_set(c, o, "0", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, 0);
        fvm_value_number(args + 1, 1);
        if (!(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "length", 0);
        if (!(fvm_object_get(c, o, "0", &value) == FVM_OK && value.type == FVM_VALUE_STRING &&
              fvm_value_string_size(&value) == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 2);
        if (!(fvm_object_set(c, o, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&value, "old");
        if (!(fvm_object_set(c, o, "0", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 9);
        if (!(fvm_object_set(c, o, "1", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(args, "1");
        args[1] = object(get_fn);
        args[2] = object(set_fn);
        if (!(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_property_flags(c, o, "1", 2, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, 0);
        fvm_value_number(args + 1, 0);
        fvm_value_string_bytes(args + 2, "a\0b", 3);
        reads = writes = watched = 0;
        if (!(borrow(c, array, "splice", o, 3, args, &out) == FVM_OK && reads == 0 && writes == 0 && watched == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, out.object, "length", 0);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(borrow(c, array, "slice", o, 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, out.object, "length", 3);
        number(c, out.object, "2", 9);
        if (!(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && value.type == FVM_VALUE_STRING &&
              fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_table_get_index(c, out.object, 1, &value) == FVM_OK && value.type == FVM_VALUE_STRING &&
              !std::strcmp(value.string, "old")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 4);
        if (!(fvm_construct(c, "Array", 1, &value, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_prototype(c, o, proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 6);
        if (!(fvm_object_set(c, o, "3", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set_property_flags(c, o, "3", 2, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, 1);
        fvm_value_number(args + 1, 1);
        if (!(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "length", 3);
        number(c, o, "3", 6);
        number(c, o, "2", 6);
        if (!(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(c, o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 3);
        if (!(fvm_object_set(c, o, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < 3; ++i)
        {
            fvm_value_number(&value, 9 - i);
            if (!(fvm_table_set_index(c, o, i, &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_number(args, 0);
        fvm_value_number(args + 1, 1);
        if (!(borrow(c, array, "splice", o, 2, args, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, o, "length", 2);
        number(c, o, "0", 8);
        number(c, o, "1", 7);
        number(c, o, "2", 7);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
