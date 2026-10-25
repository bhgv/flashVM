/* Typed native Array length retains data while only Int32 coercion controls operations. */
#include <stdio.h>
#include <stdlib.h>
#include "property_version_fixture.hpp"


static FVM_Value typed_length_input(FVM_Context *c, unsigned kind, FVM_Object *meta)
{
    FVM_Value input;
    if (kind < 4)
    {
        const char *strings[] = {"1.9", "-3.7", "4294967297", "not a number"};
        fvm_value_string(&input, strings[kind]);
    }
    else if (kind < 6)
    {
        fvm_value_boolean(&input, kind == 5);
    }
    else if (kind < 8)
    {
        fvm_value_undefined(&input);
        if (kind == 6)
        {
            input.type = FVM_VALUE_NULL;
        }
    }
    else
    {
        FVM_Object *object = 0;
        if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value fn = class_value(version_function(c, 1));
        if (!(fvm_object_set(c, object, "valueOf", &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value edge = class_value(meta);
        if (!(fvm_object_set(c, object, "edge", &edge) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return class_value(object);
    }
    input.metatable = meta;
    return input;
}


static void typed_length_equal(const FVM_Value &actual, const FVM_Value &input)
{
    if (!(actual.type == input.type))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (input.type == FVM_VALUE_STRING)
    {
        if (!(fvm_value_string_size(&actual) == fvm_value_string_size(&input) &&
              !std::memcmp(actual.string, input.string, fvm_value_string_size(&input))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (input.type == FVM_VALUE_BOOLEAN)
    {
        if (!(actual.boolean == input.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}
#ifndef FVM_TYPED_LENGTH_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const long bounds[] = {1, -3, 1, 0, 0, 1, 0, 0, 1};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned kind = 0; kind < 9; ++kind)
        {
            for (unsigned route = 0; route < 4; ++route)
            {
                for (unsigned fixed = 0; fixed < 2; ++fixed)
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
                    if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value initial[3];
                    for (unsigned j = 0; j < 3; ++j)
                    {
                        fvm_value_number(initial + j, 10 + j);
                    }
                    FVM_Object *array = 0, *meta = 0;
                    if (!(fvm_construct(c, "Array", 3, initial, &array) == FVM_OK &&
                          fvm_object_create(c, &meta) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value root = class_value(array), input = typed_length_input(c, kind, meta), out;
                    if (!(fvm_set_global(c, "typedLengthTarget", &root) == FVM_OK &&
                          fvm_object_set(c, meta, "cycle", &root) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_object_set_property_flags(c, array, "2", FVM_PROPERTY_DONT_DELETE, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_flags(c, array, "length", fixed ? 0x1007 : 0x1003, 65535);
                    if (!route)
                    {
                        if (!(fvm_object_set(c, array, "length", &input) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(fvm_set_global(c, "typedLengthInput", &input) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        Program code;
                        code.variable("typedLengthTarget");
                        code.text("length");
                        code.variable("typedLengthInput");
                        code.op(0x4f);
                        code.number(1);
                        code.finish();
                        out = boxing_run(c, code, route - 1, FVM_OK);
                    }
                    unsigned flags = 0;
                    if (!(fvm_object_property_flags(c, array, "length", &flags) == FVM_OK &&
                          flags == (fixed ? 0x1007U : 3U)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_flags(c, array, "length", 0, 0xfff8);
                    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, array, "length", &out) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (fixed)
                    {
                        if (!(out.type == FVM_VALUE_NUMBER && out.number == 3 && !out.metatable))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        typed_length_equal(out, input);
                        if (!(input.type == FVM_VALUE_OBJECT ? out.object == input.object : out.metatable == meta))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    for (unsigned j = 0; j < 3; ++j)
                    {
                        char key[2] = {(char)('0' + j), 0};
                        int own = 0;
                        if (!(fvm_object_has_own(c, array, key, &own) == FVM_OK &&
                              !!own == (j == 2 || (long)j < bounds[kind])))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!fixed)
                    {
                        if (!(fvm_object_call(c, array, "join", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
                              !std::strcmp(out.string, bounds[kind] > 0 ? "10" : "")))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_get(c, array, "length", &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        typed_length_equal(out, input);
                        unsigned char *wire = 0;
                        size_t bytes = 0;
                        if (!(fvm_value_serialize(c, &root, &wire, &bytes) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Context *other = 0;
                        if (!(fvm_context_create(rt, &other) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        version_load(other, version);
                        if (!(fvm_standard_install(other) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_value_deserialize(other, wire, bytes, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_serialized_free(wire);
                        FVM_Object *copy = out.object;
                        if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK &&
                              fvm_object_get(other, copy, "length", &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        typed_length_equal(out, input);
                        FVM_Object *copied_meta = out.metatable;
                        if (out.type == FVM_VALUE_OBJECT)
                        {
                            if (!(fvm_object_get(other, out.object, "edge", &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            copied_meta = out.object;
                        }
                        if (!(copied_meta && fvm_object_get(other, copied_meta, "cycle", &out) == FVM_OK &&
                              out.object == copy))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_call(other, copy, "join", 0, 0, &out) == FVM_OK &&
                              !std::strcmp(out.string, bounds[kind] > 0 ? "10" : "")))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        /* Old nonnumeric data never triggers tail deletion during a later length write. */
                        if (!(fvm_object_set_property_flags(c, array, "2", 0, FVM_PROPERTY_DONT_DELETE) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Value zero;
                        fvm_value_number(&zero, 0);
                        if (!(fvm_object_set(c, array, "length", &zero) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        int own = 0;
                        if (!(fvm_object_has_own(c, array, "2", &own) == FVM_OK && own))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_has_own(c, array, "0", &own) == FVM_OK && !!own == (bounds[kind] > 0)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        /* Index growth converts the transported live value and normalizes its length. */
                        FVM_Value item;
                        fvm_value_number(&item, 81);
                        if (!(fvm_object_set(other, copy, "3", &item) == FVM_OK &&
                              fvm_object_get(other, copy, "length", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
                              out.number == 4 && !out.metatable))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_context_destroy(other);
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
#endif
