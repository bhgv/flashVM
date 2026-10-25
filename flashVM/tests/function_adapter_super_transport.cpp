/* Transported adapter super facades retain fixed depth and follow the receiver's live chain. */
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
        for (unsigned chain = 0; chain < 4; ++chain)
        {
            for (unsigned apply = 0; apply < 2; ++apply)
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
                FVM_Object *nodes[4];
                FVM_Value value, out;
                for (unsigned i = 0; i < 4; ++i)
                {
                    if (!(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK && fvm_gc_pin(c, nodes[i]) == FVM_OK))
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
                for (unsigned i = 0; i < 4; ++i)
                {
                    if (!(fvm_object_set_prototype(c, nodes[i], i < chain ? nodes[i + 1] : 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                FVM_Object *function = adapter_super_function(c, 0);
                value = class_value(nodes[0]);
                if (!(fvm_object_call(c, function, apply ? "apply" : "call", 1, &value, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                unsigned char *wire = 0;
                size_t size = 0;
                if (!(fvm_value_serialize(c, &out, &wire, &size) == FVM_OK &&
                      fvm_value_deserialize(other, wire, size, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_serialized_free(wire);
                FVM_Object *facade = out.object;
                if (!(fvm_gc_pin(other, facade) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(facade->resource_kind == FVM_RESOURCE_SUPER &&
                      facade->resource_result.object->properties["depth"].number == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(other, facade, "marker", &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (chain >= 2)
                {
                    if (!(out.type == FVM_VALUE_NUMBER && out.number == 2))
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
                FVM_Object *receiver = facade->resource_result.object->properties["receiver"].object, *middle = 0,
                           *far = 0;
                if (!(fvm_construct(other, "Object", 0, 0, &middle) == FVM_OK &&
                      fvm_construct(other, "Object", 0, 0, &far) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_value_number(&value, 77);
                if (!(fvm_object_set(other, far, "marker", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_set_prototype(other, middle, far) == FVM_OK &&
                      fvm_object_set_prototype(other, receiver, middle) == FVM_OK &&
                      fvm_gc_collect(other, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(other, facade, "marker", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER &&
                      out.number == 77))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                /* Calling the transported facade selects depth-one __constructor__ and advances depth. */
                Program constructor;
                constructor.variable("this");
                constructor.text("capturedSuper");
                constructor.variable("super");
                constructor.op(0x4f);
                constructor.number(1);
                constructor.finish();
                FVM_Object *function_constructor = 0;
                if (!(fvm_function_create_avm1(
                          other, constructor.bytes.begin(), constructor.bytes.size(), 0, 0, 0, &function_constructor) ==
                      FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_set_property_flags(other, middle, "__constructor__", 0, 4) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                value = class_value(function_constructor);
                if (!(fvm_object_set(other, middle, "__constructor__", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_function_invoke(other, facade, receiver, 0, 0, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(other, receiver, "capturedSuper", &out) == FVM_OK &&
                      out.type == FVM_VALUE_OBJECT && out.object->resource_kind == FVM_RESOURCE_SUPER))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(out.object->resource_result.object->properties["depth"].number == 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!other->gc_frames && !other->gc_native_depth && !other->calling_owner &&
                      !other->calling_function))
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
