/* ASSetPropFlags stores flags under the selected own spelling, including NUL/Unicode names. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/serialization.h"
#include "../src/core/property_key.hpp"

static unsigned case_flag_getters;


static FVM_Result case_flag_getter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    ++case_flag_getters;
    return FVM_ERROR;
}


static void
case_flag_enumeration(FVM_Context *c, FVM_Object *object, const char *stored, size_t size, unsigned expected)
{
    FVM_Object *keys = 0;
    FVM_Value length, key;
    if (!(fvm_object_enumerate(c, object, 0, &keys) == FVM_OK && fvm_object_get(c, keys, "length", &length) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool found = false;
    for (unsigned long i = 0; i < (unsigned long)length.number; ++i)
    {
        if (!(fvm_table_get_index(c, keys, i, &key) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (key.type == FVM_VALUE_STRING && fvm_value_string_size(&key) == size &&
            !std::memcmp(key.string, stored, size))
        {
            found = true;
        }
    }
    if (!(found == !(expected & 1U)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    const char *stored_names[] = {
        "MiXeD", "MiXeD\0\xc3\x84", "LongPropertyNameForAllocationAndSpellingRetention_MiXeD"};
    const char *queries[] = {"mixed", "mixed\0\xc3\xa4", "longpropertynameforallocationandspellingretention_mixed"};
    const unsigned initial[] = {0, 1, 7, 4}, set[] = {2, 2, 0, 3}, clear[] = {0, 1, 7, 3};
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (unsigned type = 0; type < 3; ++type)
        {
            for (unsigned name = 0; name < 3; ++name)
            {
                for (unsigned route = 0; route < 5; ++route)
                {
                    if ((version && version < 4) || (route >= 3 && !fvm_jit_available()))
                    {
                        continue;
                    }
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (version)
                    {
                        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                        swf[3] = (unsigned char)version;
                        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_standard_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *object = 0, *parent = 0;
                    if (!(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK &&
                          fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value args[4], out, value, function;
                    fvm_value_number(&value, 9);
                    size_t size = name == 1 ? 8U : std::strlen(stored_names[name]);
                    const char *stored = stored_names[name], *query = queries[name];
                    fvm_detail::string own_key, query_key;
                    if (!(fvm_property_key(stored, size, own_key) == FVM_OK &&
                          fvm_property_key(query, size, query_key) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!type)
                    {
                        if (!(fvm_object_set_bytes(c, object, stored, size, &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (type == 1)
                    {
                        FVM_Value klass, proto;
                        if (!(fvm_get_global(c, "Object", &klass) == FVM_OK &&
                              fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        object->methods[own_key] = proto.object->methods["toString"];
                    }
                    if (type == 2)
                    {
                        if (!(fvm_object_define_accessor_rooted_bytes(
                                  c, object, stored, size, case_flag_getter, 0, 0, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_object_set(c, parent, "Parent", &value) == FVM_OK &&
                          fvm_object_set_property_flags(c, parent, "Parent", 1, 0) == FVM_OK &&
                          fvm_object_set_prototype(c, object, parent) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    args[0] = class_value(object);
                    if (!(fvm_set_global(c, "caseFlagObject", args) == FVM_OK &&
                          fvm_get_global(c, "ASSetPropFlags", &function) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_detail::vector<char> list;
                    for (size_t i = 0; i < size; ++i)
                    {
                        list.push_back(query[i]);
                    }
                    list.push_back(',');
                    for (size_t i = 0; i < size; ++i)
                    {
                        list.push_back(query[i]);
                    }
                    const char suffix[] = ",parent";
                    for (size_t i = 0; i < sizeof(suffix) - 1; ++i)
                    {
                        list.push_back(suffix[i]);
                    }
                    if (!(fvm_value_string_bytes(args + 1, list.begin(), list.size()) == FVM_OK &&
                          fvm_set_global(c, "caseFlagNames", args + 1) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    case_flag_getters = 0;
                    for (unsigned phase = 0; phase < 4; ++phase)
                    {
                        if (!(fvm_object_set_property_flags_bytes(c, object, stored, size, 0, 7) == FVM_OK &&
                              fvm_object_set_property_flags_bytes(c, object, stored, size, initial[phase], 0) ==
                                  FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_value_number(args + 2, set[phase]);
                        fvm_value_number(args + 3, clear[phase]);
                        if (!route)
                        {
                            if (!(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (route == 1)
                        {
                            if (!(fvm_function_invoke(c, function.object, 0, 4, args, &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            class_mode = route - 2;
                            Program code;
                            code.number(clear[phase]);
                            code.number(set[phase]);
                            code.variable("caseFlagNames");
                            code.variable("caseFlagObject");
                            code.number(4);
                            code.text("ASSetPropFlags");
                            code.op(0x3d);
                            code.finish();
                            out = class_run(c, code);
                        }
                        unsigned expected = version ? (initial[phase] & ~clear[phase]) | set[phase] : initial[phase],
                                 actual = 99;
                        if (!(out.type == FVM_VALUE_UNDEFINED &&
                              fvm_object_property_flags_bytes(c, object, stored, size, &actual) == FVM_OK &&
                              actual == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(object->attributes.find(query_key) == object->attributes.end()))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(expected ? object->attributes.find(own_key) != object->attributes.end() &&
                                             object->attributes.find(own_key)->second == expected
                                       : object->attributes.find(own_key) == object->attributes.end()))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_property_flags(c, parent, "Parent", &actual) == FVM_OK && actual == 1 &&
                              object->attributes.find("parent") == object->attributes.end()))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        case_flag_enumeration(c, object, stored, size, expected);
                        if (!(!case_flag_getters))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (type != 2)
                        {
                            FVM_Context *other = 0;
                            if (!(fvm_context_create(rt, &other) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            const unsigned char swf8[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
                            unsigned char *data = 0;
                            size_t bytes = 0;
                            if (!(fvm_load_memory(other, swf8, sizeof(swf8)) == FVM_OK &&
                                  fvm_value_serialize(c, args, &data, &bytes) == FVM_OK &&
                                  fvm_value_deserialize(other, data, bytes, &value) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            fvm_serialized_free(data);
                            if (!(fvm_gc_pin(other, value.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK &&
                                  fvm_object_property_flags_bytes(other, value.object, stored, size, &actual) ==
                                      FVM_OK &&
                                  actual == expected))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            case_flag_enumeration(other, value.object, stored, size, expected);
                            fvm_context_destroy(other);
                        }
                    }
                    if (!(!c->gc_frames && !c->gc_native_depth && !case_flag_getters))
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
