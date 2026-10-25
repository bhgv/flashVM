/* Pure transported bytecode mutates destination-local this values without serializing live cells. */
#define FVM_THIS_ASSIGNMENT_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "this_cell_assignment.cpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (unsigned type = 0; type < 3; ++type)
        {
            for (unsigned route = 0; route < 4; ++route)
            {
                if (route >= 2 && !fvm_jit_available())
                {
                    continue;
                }
                FVM_Context *c = 0, *other = 0;
                if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                version_load(c, version);
                version_load(other, version);
                if (!(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *bundle = 0, *meta = 0;
                if (!(fvm_object_create(c, &bundle) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value value = class_value(assigned_this_function(c, "this", 0));
                if (!(fvm_object_set(c, bundle, "function", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!type)
                {
                    value = class_value(meta);
                }
                else
                {
                    if (type == 1)
                    {
                        fvm_value_string_bytes(&value, "t\0s", 3);
                    }
                    else
                    {
                        fvm_value_number(&value, 27);
                    }
                    value.metatable = meta;
                }
                if (!(fvm_object_set(c, bundle, "replacement", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                value = class_value(bundle);
                unsigned char *wire = 0;
                size_t size = 0;
                if (!(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK &&
                      fvm_value_deserialize(other, wire, size, &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_serialized_free(wire);
                bundle = value.object;
                if (!(fvm_gc_pin(other, bundle) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value replacement, function;
                if (!(fvm_object_get(other, bundle, "replacement", &replacement) == FVM_OK &&
                      fvm_object_get(other, bundle, "function", &function) == FVM_OK &&
                      fvm_set_global(other, "replacement", &replacement) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                value = this_path_invoke(other, function.object, false, route);
                boxing_equal(value, replacement);
                if (!(value.metatable == replacement.metatable))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(other, 0) == FVM_OK && !other->script_this_cell && !other->script_this &&
                      !other->gc_frames))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(other);
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
