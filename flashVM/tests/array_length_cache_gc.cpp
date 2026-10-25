/* Deleted or replaced native length releases obsolete object and primitive-metatable graphs. */
#define FVM_TYPED_LENGTH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_typed_length.cpp"
#include "../src/standard/base_state.hpp"


static bool cache_gc_alive(FVM_Context *c, FVM_Object *o)
{
    for (size_t i = 0; i < c->objects.size(); ++i)
    {
        if (c->objects[i] == o)
        {
            return true;
        }
    }
    return false;
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
        for (unsigned kind = 0; kind < 10; ++kind)
        {
            for (unsigned operation = 0; operation < 4; ++operation)
            {
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
                FVM_Object *array = 0, *meta = 0;
                if (!(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK &&
                      fvm_object_create(c, &meta) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value input, root = class_value(array), out;
                if (!(fvm_object_set(c, meta, "cycle", &root) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (kind < 9)
                {
                    input = typed_length_input(c, kind, meta);
                }
                else
                {
                    fvm_value_number(&input, 1);
                    input.metatable = meta;
                }
                FVM_Object *object = input.type == FVM_VALUE_OBJECT ? input.object : 0;
                if (!(fvm_object_set(c, array, "length", &input) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK &&
                      cache_gc_alive(c, meta)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (operation == 3)
                {
                    if (!(fvm_object_set(c, array, "retainedLength", &input) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (operation == 2)
                { /* An alias of the native callback is not the effective length property. */
                    array->accessors["alias"] = array->accessors.find("length")->second;
                    int removed = 0;
                    if (!(fvm_object_delete(c, array, "alias", &removed) == FVM_OK && removed &&
                          fvm_gc_collect(c, 0) == FVM_OK && cache_gc_alive(c, meta)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (operation == 1)
                {
                    FVM_Value args[3];
                    fvm_value_string(args, "length");
                    args[1] = class_value(version_function(c, 9));
                    args[2] = class_value(version_function(c, 8));
                    if (!(fvm_object_call(c, array, "addProperty", 3, args, &out) == FVM_OK && out.boolean &&
                          fvm_gc_collect(c, 0) == FVM_OK && cache_gc_alive(c, meta)))
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
                }
                else
                {
                    if (!(fvm_object_set_property_flags(c, array, "length", 0, FVM_PROPERTY_DONT_DELETE) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    int removed = 0;
                    const char *name = version < 7 ? "LeNgTh" : "length";
                    if (!(fvm_object_delete(c, array, name, &removed) == FVM_OK && removed &&
                          fvm_object_get(c, array, "length", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (operation == 3)
                {
                    if (!(fvm_gc_collect(c, 0) == FVM_OK && cache_gc_alive(c, meta) &&
                          (!object || cache_gc_alive(c, object))))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    int removed = 0;
                    if (!(fvm_object_delete(c, array, "retainedLength", &removed) == FVM_OK && removed))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK && !cache_gc_alive(c, meta) &&
                      (!object || !cache_gc_alive(c, object))))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(array->resource_result.type == FVM_VALUE_NUMBER && !array->resource_result.number &&
                      !array->resource_result.metatable))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth))
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
