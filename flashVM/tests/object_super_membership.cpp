/* Loaded super own-membership forwards to its retained receiver, including after graph transfer. */
#define main super_membership_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_super.cpp"

#undef main
#include "../src/core/runtime.hpp"


static void check_membership(FVM_Context *c, FVM_Object *facade, FVM_Object *receiver, bool loaded, unsigned version)
{
    FVM_Value value, klass, proto, own_function, args[2], out;
    fvm_value_number(&value, 77);
    if (!(fvm_object_set_bytes(c, receiver, "MiXeD\0Name", 10, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "Object", &klass) == FVM_OK &&
          fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, proto.object, "hasOwnProperty", &own_function) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char trap[] = {0x96, 6, 0, 0, 't', 'r', 'a', 'p', 0, 0x2a, 0};
    FVM_Object *get = 0;
    if (!(fvm_function_create_avm1(c, trap, sizeof(trap), 0, 0, 0, &get) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value descriptor[3];
    fvm_value_string(descriptor, "Getter");
    fvm_value_undefined(descriptor + 1);
    descriptor[1].type = FVM_VALUE_OBJECT;
    descriptor[1].object = get;
    fvm_value_undefined(descriptor + 2);
    descriptor[2].type = FVM_VALUE_NULL;
    FVM_Value add_function, borrowed[4];
    if (!(fvm_object_get(c, proto.object, "addProperty", &add_function) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(borrowed);
    borrowed[0].type = FVM_VALUE_OBJECT;
    borrowed[0].object = receiver;
    for (unsigned i = 0; i < 3; ++i)
    {
        borrowed[i + 1] = descriptor[i];
    }
    if (!(fvm_object_call(c, add_function.object, "call", 4, borrowed, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *keys[] = {"MiXeD\0Name", "mixed\0name", "inherited", "missing", "Getter", "getter"};
    const size_t sizes[] = {10, 10, 9, 7, 6, 6};
    for (unsigned i = 0; i < 6; ++i)
    {
        const bool expected = loaded && (i == 0 || i == 4 || ((i == 1 || i == 5) && version <= 6));
        int own = 0;
        if (!(fvm_object_has_own_bytes(c, facade, keys[i], sizes[i], &own) == FVM_OK && !!own == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(args);
        args[0].type = FVM_VALUE_OBJECT;
        args[0].object = facade;
        fvm_value_string_bytes(args + 1, keys[i], sizes[i]);
        if (!(fvm_object_call(c, own_function.object, "call", 2, args, &out) == FVM_OK && !!out.boolean == expected))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    /* The accessor is executable and throwing; membership must not have read it. */
    if (!(fvm_object_get(c, receiver, "Getter", &out) == FVM_EXCEPTION && fvm_exception_clear(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (loaded)
    {
        FVM_Value saved = facade->resource_result.object->properties["receiver"];
        FVM_Value cycle;
        fvm_value_undefined(&cycle);
        cycle.type = FVM_VALUE_OBJECT;
        cycle.object = facade;
        facade->resource_result.object->properties["receiver"] = cycle;
        int own = 0;
        if (!(fvm_object_has_own(c, facade, "missing", &own) == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        facade->resource_result.object->properties["receiver"] = saved;
        if (!(fvm_object_has_own_bytes(c, facade, keys[0], sizes[0], &own) == FVM_OK && own))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
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
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 5)
        {
            continue;
        }
        FVM_Context *c = 0, *other = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes empty;
        empty.push_back(0);
        FVM_Value base = define(c, "Base", 0, 0, empty), sub = define(c, "Sub", 0, 0, empty);
        extend(c, "Sub", "Base");
        FVM_Value value;
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, prototype(c, base), "inherited", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes escape;
        variable(escape, "super");
        result(escape);
        FVM_Value method = define(c, "escape", 0, 0, escape);
        if (!(fvm_object_set(c, prototype(c, sub), "escape", &method) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *receiver = 0;
        if (!(fvm_function_construct(c, sub.object, 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value facade;
        if (!(fvm_object_call(c, receiver, "escape", 0, 0, &facade) == FVM_OK && facade.type == FVM_VALUE_OBJECT &&
              fvm_gc_pin(c, facade.object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        check_membership(c, facade.object, receiver, version != 0, version);
        FVM_Channel *channel = 0;
        if (!(fvm_channel_create(&channel) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_send_value(c, channel, &facade, 0) == FVM_OK &&
              fvm_channel_receive_value(other, channel, &facade, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(other, facade.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *copy_receiver = facade.object->resource_result.object->properties["receiver"].object;
        check_membership(other, facade.object, copy_receiver, version != 0, version);
        fvm_channel_destroy(channel);
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
