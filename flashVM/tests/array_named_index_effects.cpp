/* Watch-before-growth, inherited/own virtual dispatch, and overflow budget cleanup. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"

static unsigned calls;


static FVM_Result named_watch(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *a, FVM_Value *out, void *)
{
    if (!(n == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++calls;
    FVM_Value length;
    fvm_value_number(&length, -3);
    if (!(fvm_object_set(c, self, "length", &length) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    *out = a[2];
    return FVM_OK;
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
        FVM_Object *array = 0, *watch = 0;
        FVM_Value args[4], out;
        if (!(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_native(c, named_watch, 0, 0, &watch) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(args, "-1");
        args[1] = class_value(watch);
        if (!(fvm_object_call(c, array, "watch", 2, args, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        calls = 0;
        fvm_value_number(args, 81);
        if (!(fvm_object_set(c, array, "-1", args) == FVM_OK && calls == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, array, "length", &out) == FVM_OK && out.number == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Counted NUL and non-ASCII whitespace are ordinary literal properties. */
        if (!(fvm_object_set_bytes(c, array, "1\0x", 3, args) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c,
                             array,
                             "\xc2\xa0"
                             "1",
                             args) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, array, "length", &out) == FVM_OK && out.number == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned inherited = 0; inherited < 2; ++inherited)
        {
            FVM_Object *target = 0, *owner = 0, *setter = 0;
            if (!(fvm_construct(c, "Array", 0, 0, &target) == FVM_OK && fvm_gc_pin(c, target) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            owner = target;
            if (inherited)
            {
                if (!(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_set_prototype(c, target, owner) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            const unsigned char body[] = {0x96, 6,   0,   0,   't', 'h', 'i', 's', 0, 0x1c, 0x96, 13, 0,    0,
                                          'c',  'a', 'l', 'l', 'e', 'd', 0,   7,   1, 0,    0,    0,  0x4f, 0};
            if (!(fvm_function_create_avm1(c, body, sizeof(body), 0, 0, 0, &setter) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(args, "+01");
            args[1] = class_value(version_function(c, 42));
            args[2] = class_value(setter);
            if (!(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(args, 81);
            if (!(fvm_object_set(c, target, "+01", args) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, target, "length", &out) == FVM_OK && out.number == (inherited ? 0 : 2)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, target, "called", &out) == FVM_OK && out.number == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        FVM_Object *budget = 0;
        fvm_value_number(args, 3);
        if (!(fvm_construct(c, "Array", 1, args, &budget) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, 81);
        for (unsigned long i = 0; i < 3; ++i)
        {
            if (!(fvm_table_set_index(c, budget, i, args) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        const unsigned long limit = rt->config.instruction_limit;
        rt->config.instruction_limit = 1;
        if (!(fvm_object_set(c, budget, "+2147483647", args) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = limit;
        if (!(fvm_object_get(c, budget, "length", &out) == FVM_OK && out.number == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int own = 0;
        if (!(fvm_object_has_own(c, budget, "+2147483647", &own) == FVM_OK && !own))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_set(c, budget, "+2147483647", args) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, budget, "length", &out) == FVM_OK && out.number == -2147483648.0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!c->gc_frames && !c->gc_native_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
