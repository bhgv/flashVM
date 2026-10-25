/* Exact length assignment prepares truncation before ordinary/virtual descriptor dispatch. */
#define FVM_TYPED_LENGTH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_typed_length.cpp"
#include "../src/standard/base_state.hpp"


static FVM_Object *reassignment_array(FVM_Context *c, unsigned kind)
{
    FVM_Value initial[3];
    for (unsigned j = 0; j < 3; ++j)
    {
        fvm_value_number(initial + j, 10 + j);
    }
    FVM_Object *array = 0;
    if (!(fvm_construct(c, "Array", 3, initial, &array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (kind != 4)
    {
        if (!(fvm_object_set_property_flags(c, array, "length", 0, FVM_PROPERTY_DONT_DELETE) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        int removed = 0;
        if (!(fvm_object_delete(c, array, "length", &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (kind && kind != 4)
    {
        FVM_Value length;
        if (kind == 2)
        {
            fvm_value_string(&length, "3");
        }
        else
        {
            fvm_value_number(&length, 3);
        }
        if (!(fvm_object_set(c, array, "length", &length) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (kind >= 3)
    {
        FVM_Value args[3], out;
        fvm_value_string(args, "length");
        args[1] = class_value(version_function(c, 99));
        args[2] = class_value(version_function(c, 88));
        if (!(fvm_object_call(c, array, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return array;
}


static FVM_Value reassignment_raw(FVM_Context *c, FVM_Object *array, unsigned kind)
{
    FVM_Value value;
    if (kind >= 3)
    {
        if (!(fvm_base_accessor_data(array->accessors.find("length")->second, value)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(fvm_object_get(c, array, "length", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return value;
}


static FVM_Result reassignment_write(
    FVM_Context *c, FVM_Object *array, const FVM_Value &input, unsigned route, FVM_Result expected = FVM_OK)
{
    FVM_Value root = class_value(array), out;
    if (!(fvm_set_global(c, "reassignmentTarget", &root) == FVM_OK &&
          fvm_set_global(c, "reassignmentInput", &input) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!route)
    {
        return fvm_object_set(c, array, "length", &input);
    }
    Program p;
    p.variable("reassignmentTarget");
    p.text("length");
    p.variable("reassignmentInput");
    p.op(0x4f);
    p.number(1);
    p.finish();
    out = boxing_run(c, p, route - 1, expected);
    (void)out;
    return expected;
}
#ifndef FVM_REASSIGNMENT_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned kinds[] = {0, 4, 6, 7, 8};
    const long bounds[] = {1, 0, 0, 0, 1};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned kind = 0; kind < 5; ++kind)
        {
            for (unsigned value = 0; value < 5; ++value)
            {
                for (unsigned fixed = 0; fixed < 2; ++fixed)
                {
                    for (unsigned route = 0; route < 4; ++route)
                    {
                        for (unsigned transport = 0; transport < 2; ++transport)
                        {
                            if ((!kind && fixed) || (route >= 2 && !fvm_jit_available()))
                            {
                                continue;
                            }
                            FVM_Context *c = 0, *source = 0;
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
                            FVM_Object *array = reassignment_array(c, kind), *meta = 0;
                            if (!(fvm_object_set_property_flags(c, array, "2", FVM_PROPERTY_DONT_DELETE, 0) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (fixed)
                            {
                                if (!(fvm_object_set_property_flags(c, array, "length", FVM_PROPERTY_READ_ONLY, 0) ==
                                      FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if (transport)
                            {
                                FVM_Value root = class_value(array), out;
                                unsigned char *wire = 0;
                                size_t bytes = 0;
                                source = c;
                                if (!(fvm_value_serialize(source, &root, &wire, &bytes) == FVM_OK &&
                                      fvm_context_create(rt, &c) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                version_load(c, version);
                                if (!(fvm_standard_install(c) == FVM_OK &&
                                      fvm_value_deserialize(c, wire, bytes, &out) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                fvm_serialized_free(wire);
                                array = out.object;
                            }
                            if (!(fvm_gc_pin(c, array) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Value input = typed_length_input(c, kinds[value], meta),
                                      old = reassignment_raw(c, array, kind);
                            if (!(reassignment_write(c, array, input, route) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Value raw = reassignment_raw(c, array, kind);
                            if (fixed)
                            {
                                boxing_equal(raw, old);
                            }
                            else
                            {
                                typed_length_equal(raw, input);
                                if (!(input.type == FVM_VALUE_OBJECT ? raw.object == input.object
                                                                     : raw.metatable == meta))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            for (unsigned j = 0; j < 3; ++j)
                            {
                                char key[2] = {(char)('0' + j), 0};
                                int own = 0;
                                const bool numeric_old = kind == 1 || kind >= 3;
                                if (!(fvm_object_has_own(c, array, key, &own) == FVM_OK &&
                                      !!own == (!numeric_old || j == 2 || (long)j < bounds[value])))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth &&
                                  !c->script_depth && !c->exception_pending))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            fvm_context_destroy(c);
                            if (source)
                            {
                                fvm_context_destroy(source);
                            }
                        }
                    }
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
#endif
