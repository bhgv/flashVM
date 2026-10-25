/* A transported super facade preserves inert writes and counted-name parent setter dispatch. */
#define FVM_SUPER_WRITE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "super_property_write.cpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *keys[] = {"slot", "5", "+5", "n\0m"};
    const size_t sizes[] = {4, 1, 2, 3};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned array = 0; array < 2; ++array)
        {
            for (unsigned key = 0; key < 4; ++key)
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
                FVM_Object *receiver = 0, *parent = 0, *middle = 0, *set = 0;
                if (!(fvm_construct(c, array ? "Array" : "Object", 0, 0, &receiver) == FVM_OK &&
                      fvm_gc_pin(c, receiver) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK &&
                      fvm_construct(c, "Object", 0, 0, &middle) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program body;
                body.variable("this");
                body.text("called");
                body.variable("input");
                body.op(0x4f);
                body.number(1);
                body.finish();
                const char *parameters[] = {"input"};
                if (!(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 1, parameters, 0, &set) ==
                      FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                super_write_property(c, parent, keys[key], sizes[key], version_function(c, 42), set);
                FVM_Value value, out;
                fvm_value_number(&value, 9);
                if (!(fvm_object_set_bytes(c, middle, keys[key], sizes[key], &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_set_prototype(c, parent, receiver->prototype) == FVM_OK &&
                      fvm_object_set_prototype(c, middle, parent) == FVM_OK &&
                      fvm_object_set_prototype(c, receiver, middle) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *facade = super_write_escape(c, receiver);
                value = class_value(facade);
                unsigned char *wire = 0;
                size_t size = 0;
                if (!(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK &&
                      fvm_value_deserialize(other, wire, size, &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_serialized_free(wire);
                FVM_Object *copy = out.object;
                if (!(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *target = copy->resource_result.object->properties["receiver"].object;
                fvm_value_string_bytes(&value, "r\0s", 3);
                if (!(fvm_object_set_bytes(other, copy, keys[key], sizes[key], &value) == FVM_OK &&
                      fvm_gc_collect(other, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_get(other, target, "called", &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
                      fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "r\0s", 3)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                int own = 1;
                if (!(fvm_object_has_own_bytes(other, target, keys[key], sizes[key], &own) == FVM_OK && !own))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (array)
                {
                    if (!(fvm_object_get(other, target, "length", &out) == FVM_OK && out.number == 0))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                fvm_value_number(&value, 2);
                if (!(fvm_object_set(other, copy, "length", &value) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_has_own(other, target, "length", &own) == FVM_OK && !!own == !!array))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(!other->gc_frames && !other->gc_native_depth && !other->calling_setter_owner))
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
