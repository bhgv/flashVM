/* Borrowed definition/watch methods validate names but do not mutate loaded super facades. */
#define main super_definition_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "avm1_super.cpp"

#undef main
#include "../src/core/runtime.hpp"

static unsigned super_name_calls, super_name_fault;


static FVM_Result super_name(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++super_name_calls;
    if (super_name_fault)
    {
        fvm_value_number(out, 91);
        return fvm_exception_raise(c, out);
    }
    return fvm_value_string_bytes(out, "N\0M", 3);
}


static FVM_Value super_value(FVM_Object *object)
{
    FVM_Value value;
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = object;
    return value;
}


static FVM_Result super_borrow(FVM_Context *c,
                               FVM_Object *prototype,
                               const char *method,
                               FVM_Object *receiver,
                               unsigned n,
                               const FVM_Value *args,
                               FVM_Value *out)
{
    FVM_Value function, passed[4];
    if (!(n < 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, prototype, method, &function) == FVM_OK && function.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    passed[0] = super_value(receiver);
    for (unsigned i = 0; i < n; ++i)
    {
        passed[i + 1] = args[i];
    }
    return fvm_object_call(c, function.object, "call", n + 1, passed, out);
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
        Bytes empty;
        empty.push_back(0);
        FVM_Value base = define(c, "Base", 0, 0, empty), sub = define(c, "Sub", 0, 0, empty);
        extend(c, "Sub", "Base");
        Bytes escape;
        variable(escape, "super");
        result(escape);
        FVM_Value method = define(c, "escape", 0, 0, escape);
        if (!(fvm_object_set(c, prototype(c, sub), "escape", &method) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *instance = 0, *key = 0, *handler = 0;
        FVM_Value facade, klass, proto, args[3], out;
        if (!(fvm_function_construct(c, sub.object, 0, 0, &instance) == FVM_OK && fvm_gc_pin(c, instance) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, instance, "escape", 0, 0, &facade) == FVM_OK && facade.type == FVM_VALUE_OBJECT &&
              fvm_gc_pin(c, facade.object) == FVM_OK))
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
        if (!(fvm_construct(c, "Object", 0, 0, &key) == FVM_OK && fvm_gc_pin(c, key) == FVM_OK &&
              fvm_object_define_method_rooted(c, key, "toString", super_name, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char end[] = {0};
        if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &handler) == FVM_OK &&
              fvm_gc_pin(c, handler) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            args[0] = super_value(key);
        }
        else
        {
            fvm_value_string_bytes(args, "N\0M", 3);
        }
        args[1] = super_value(handler);
        fvm_value_undefined(args + 2);
        args[2].type = FVM_VALUE_NULL;
        super_name_calls = super_name_fault = 0;
        if (!(super_borrow(c, proto.object, "addProperty", facade.object, 3, args, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(facade.object->accessors.size() == (version ? 0U : 1U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!version)
        {
            fvm_value_string(args, "Watch");
        }
        if (!(super_borrow(c, proto.object, "watch", facade.object, 2, args, &out) == FVM_OK && out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(facade.object->watchers.size() == (version ? 0U : 1U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(super_borrow(c, proto.object, "unwatch", facade.object, 1, args, &out) == FVM_OK &&
              !!out.boolean == !version))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(super_name_calls == (version ? 3U : 0U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            args[1].type = FVM_VALUE_NULL;
            if (!(super_borrow(c, proto.object, "addProperty", facade.object, 3, args, &out) == FVM_OK &&
                  !out.boolean && super_name_calls == 4))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(super_borrow(c, proto.object, "watch", facade.object, 2, args, &out) == FVM_OK && !out.boolean &&
                  super_name_calls == 5))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            args[1] = super_value(handler);
            super_name_fault = 1;
            const char *methods[] = {"addProperty", "watch", "unwatch"};
            for (unsigned i = 0; i < 3; ++i)
            {
                if (!(super_borrow(c,
                                   proto.object,
                                   methods[i],
                                   facade.object,
                                   i == 0   ? 3
                                   : i == 1 ? 2
                                            : 1,
                                   args,
                                   &out) == FVM_EXCEPTION))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == 91 && fvm_exception_clear(c) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            super_name_fault = 0;
        }
        fvm_value_string(args, "regular");
        args[1] = facade;
        if (!(super_borrow(c, proto.object, "watch", instance, 2, args, &out) == FVM_OK && !!out.boolean == !version))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
