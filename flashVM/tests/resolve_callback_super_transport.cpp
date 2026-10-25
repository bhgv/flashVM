/* Serialized pure AVM1 resolvers preserve depth zero and counted property keys. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_adapter_super.cpp"


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
        for (unsigned chain = 0; chain < 3; ++chain)
        {
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
            FVM_Object *nodes[3];
            FVM_Value args[2], value, out;
            for (unsigned i = 0; i < 3; ++i)
            {
                if (!(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_number(&value, i);
                if (!(fvm_object_set(c, nodes[i], "marker", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            fvm_value_string_bytes(args, "n\0m", 3);
            args[1] = class_value(adapter_super_function(c, 0));
            if (!(fvm_object_set(c, nodes[0], "__resolve", args + 1) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned i = 0; i < 3; ++i)
            {
                if (!(fvm_object_set_prototype(c, nodes[i], i < chain ? nodes[i + 1] : 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            value = class_value(nodes[0]);
            unsigned char *wire = 0;
            size_t size = 0;
            if (!(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK &&
                  fvm_value_deserialize(other, wire, size, &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_serialized_free(wire);
            FVM_Object *receiver = out.object;
            if (!(fvm_gc_pin(other, receiver) == FVM_OK && fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK &&
                  fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }


            if (!(fvm_object_get_bytes(other, receiver, "n\0m", 3, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT &&
                  out.object->resource_result.object->properties["depth"].number == 0))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(other, out.object, "marker", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (chain)
            {
                if (!(out.type == FVM_VALUE_NUMBER && out.number == 1))
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
            if (!(fvm_gc_collect(other, 0) == FVM_OK && !other->resolve_depth && !other->gc_frames &&
                  !other->gc_native_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(other);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
