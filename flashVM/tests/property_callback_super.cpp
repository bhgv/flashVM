/* Script property getters/setters use fixed superclass depth independently of descriptor owner. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "function_adapter_super.cpp"
#include "../src/standard/base_state.hpp"


static void callback_super_property(
    FVM_Context *c, FVM_Object *owner, const char *key, size_t size, FVM_Object *get, FVM_Object *set)
{
    FVM_Value args[3], out;
    fvm_value_string_bytes(args, key, size);
    args[1] = class_value(get);
    args[2] = class_value(set);
    if (!(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_PROPERTY_CALLBACK_SUPER_FIXTURE_ONLY


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
            for (unsigned chain = 0; chain < 4; ++chain)
            {
                for (unsigned location = 0; location <= chain; ++location)
                {
                    for (unsigned key = 0; key < 4; ++key)
                    {
                        for (unsigned setter = 0; setter < 2; ++setter)
                        {
                            for (unsigned readonly = 0; readonly < 2; ++readonly)
                            {
                                for (unsigned route = 0; route < 4; ++route)
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
                                    if (!(fvm_standard_install(c) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                    FVM_Object *nodes[4];
                                    FVM_Value value, out;
                                    for (unsigned i = 0; i < 4; ++i)
                                    {
                                        if (!(fvm_construct(c, !i && array ? "Array" : "Object", 0, 0, nodes + i) ==
                                                  FVM_OK &&
                                              fvm_gc_pin(c, nodes[i]) == FVM_OK))
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
                                    FVM_Object *getter = adapter_super_function(c, 0), *write = 0;
                                    Program body;
                                    body.variable("this");
                                    body.text("savedSuper");
                                    body.variable("super");
                                    body.op(0x4f);
                                    body.number(1);
                                    body.finish();
                                    if (!(fvm_function_create_avm1(
                                              c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &write) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                    callback_super_property(c, nodes[location], keys[key], sizes[key], getter, write);
                                    if (readonly)
                                    {
                                        nodes[location]->attributes[fvm_detail::string(keys[key], sizes[key])] = 4;
                                    }
                                    for (unsigned i = 0; i < 4; ++i)
                                    {
                                        if (!(fvm_object_set_prototype(c, nodes[i], i < chain ? nodes[i + 1] : 0) ==
                                              FVM_OK))
                                        {
                                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                            exit(EXIT_FAILURE);
                                        }
                                    }
                                    value = class_value(nodes[0]);
                                    if (!(fvm_set_global(c, "callbackSuperTarget", &value) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                    fvm_value_string_bytes(&value, keys[key], sizes[key]);
                                    if (!(fvm_set_global(c, "callbackSuperKey", &value) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                    if (!route)
                                    {
                                        if (setter)
                                        {
                                            fvm_value_number(&value, 81);
                                            if (!(fvm_object_set_bytes(c, nodes[0], keys[key], sizes[key], &value) ==
                                                  FVM_OK))
                                            {
                                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                                exit(EXIT_FAILURE);
                                            }
                                        }
                                        else
                                        {
                                            if (!(fvm_object_get_bytes(c, nodes[0], keys[key], sizes[key], &out) ==
                                                  FVM_OK))
                                            {
                                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                                exit(EXIT_FAILURE);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        Program code;
                                        code.variable("callbackSuperTarget");
                                        code.variable("callbackSuperKey");
                                        if (setter)
                                        {
                                            code.number(81);
                                            code.op(0x4f);
                                            code.number(1);
                                        }
                                        else
                                        {
                                            code.op(0x4e);
                                        }
                                        code.finish();
                                        out = boxing_run(c, code, route - 1, FVM_OK);
                                    }
                                    if (setter)
                                    {
                                        if (!(fvm_object_get(c, nodes[0], "savedSuper", &out) == FVM_OK))
                                        {
                                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                            exit(EXIT_FAILURE);
                                        }
                                        FVM_Value data;
                                        if (!(fvm_base_accessor_data(
                                                nodes[location]
                                                    ->accessors.find(fvm_detail::string(keys[key], sizes[key]))
                                                    ->second,
                                                data)))
                                        {
                                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                            exit(EXIT_FAILURE);
                                        }
                                        if (!location && !readonly)
                                        {
                                            if (!(data.type == FVM_VALUE_NUMBER && data.number == 81))
                                            {
                                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                                exit(EXIT_FAILURE);
                                            }
                                        }
                                        else
                                        {
                                            if (!(data.type == FVM_VALUE_UNDEFINED))
                                            {
                                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                                exit(EXIT_FAILURE);
                                            }
                                        }
                                    }
                                    if (!(out.type == FVM_VALUE_OBJECT &&
                                          out.object->resource_kind == FVM_RESOURCE_SUPER &&
                                          out.object->resource_result.object->properties["depth"].number == 1))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                    FVM_Object *facade = out.object;
                                    if (!(fvm_gc_pin(c, facade) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                    if (!(fvm_object_get(c, facade, "marker", &out) == FVM_OK))
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
                                    if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_function &&
                                          !c->calling_owner && !c->calling_setter_owner))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                    fvm_context_destroy(c);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    /* Unloaded getters/setters retain owner-derived superclass depth. */
    for (unsigned setter = 0; setter < 2; ++setter)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *receiver = 0, *parent = 0, *write = 0;
        FVM_Value value, out;
        if (!(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK &&
              fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 1);
        if (!(fvm_object_set(c, parent, "marker", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Program body;
        body.variable("this");
        body.text("savedSuper");
        body.variable("super");
        body.op(0x4f);
        body.number(1);
        body.finish();
        if (!(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &write) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        callback_super_property(c, receiver, "slot", 4, adapter_super_function(c, 0), write);
        if (!(fvm_object_set_prototype(c, receiver, parent) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (setter)
        {
            fvm_value_number(&value, 81);
            if (!(fvm_object_set(c, receiver, "slot", &value) == FVM_OK &&
                  fvm_object_get(c, receiver, "savedSuper", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(fvm_object_get(c, receiver, "slot", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(out.type == FVM_VALUE_OBJECT && out.object->resource_result.object->properties["depth"].number == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, out.object, "marker", &out) == FVM_OK && out.number == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}

#endif
