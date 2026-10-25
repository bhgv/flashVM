/* Graph-restored explicit-super methods retain facade-relative selected method depth. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_adapter_super.cpp"
#include "../src/standard/super.hpp"
#include "../src/core/call_scope.hpp"


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
        for (unsigned base = 0; base < 3; ++base)
        {
            for (unsigned parent_call = 0; parent_call < 3; ++parent_call)
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
                FVM_Object *nodes[7], *resolver = 0;
                FVM_Value value, out;
                for (unsigned i = 0; i < 7; ++i)
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
                FVM_Object *method = adapter_super_function(c, 0);
                value = class_value(method);
                if (!(fvm_object_set_bytes(c, nodes[base + parent_call + 1], "n\0m", 3, &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                for (unsigned i = 0; i < 7; ++i)
                {
                    if (!(fvm_object_set_prototype(c, nodes[i], i < 6 ? nodes[i + 1] : 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                {


                    FVM_CallScope scope(c, 0, method, 0, base);
                    if (!(fvm_super_create(c, method, nodes[0], &value) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                const unsigned depth = base + parent_call + 1;
                unsigned char *wire = 0;
                size_t size = 0;
                if (!(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK &&
                      fvm_value_deserialize(other, wire, size, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_serialized_free(wire);
                FVM_Object *target = out.object;
                if (!(fvm_gc_pin(other, target) == FVM_OK &&
                      fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_call_bytes(other, target, "n\0m", 3, 0, 0, &out) == FVM_OK &&
                      out.type == FVM_VALUE_OBJECT &&
                      out.object->resource_result.object->properties["depth"].number == depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_pin(other, out.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK &&
                      fvm_object_get(other, out.object, "marker", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
                      out.number == depth + 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!other->resolve_depth && !other->gc_frames && !other->gc_native_depth))
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
