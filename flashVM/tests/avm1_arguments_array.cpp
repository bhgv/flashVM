#include <stdio.h>
#include <stdlib.h>
#include "flashvm/avm1.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/table.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "flashvm/property.h"
#include "../src/core/runtime.hpp"
#include <cstring>

typedef fvm_detail::vector<unsigned char> Bytes;


static void u16(Bytes &b, unsigned n)
{
    b.push_back((unsigned char)n);
    b.push_back((unsigned char)(n >> 8));
}


static void text(Bytes &b, const char *s)
{
    b.push_back(0x96);
    u16(b, (unsigned)std::strlen(s) + 2);
    b.push_back(0);
    do
    {
        b.push_back((unsigned char)*s);
    } while (*s++);
}


static void variable(Bytes &b, const char *s)
{
    text(b, s);
    b.push_back(0x1c);
}


static void integer(Bytes &b, unsigned n)
{
    b.push_back(0x96);
    u16(b, 5);
    b.push_back(7);
    for (unsigned i = 0; i < 4; ++i)
    {
        b.push_back((unsigned char)n);
        n >>= 8;
    }
}


static FVM_Object *make(FVM_Context *c, Bytes &b, unsigned params)
{
    b.push_back(0x3e);
    b.push_back(0);
    const char *name = "x";
    FVM_Object *fn = 0;
    if (!(fvm_function_create_avm1(c, b.begin(), b.size(), params, params ? &name : 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fn;
}

static unsigned traps;


static FVM_Result trap(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{
    ++traps;
    return FVM_ERROR;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        FVM_Context *c = 0, *other = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK))
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
        FVM_Value arg, out;
        fvm_value_number(&arg, 7);
        Bytes assign_parameter;
        text(assign_parameter, "x");
        integer(assign_parameter, 99);
        assign_parameter.push_back(0x1d);
        variable(assign_parameter, "arguments");
        integer(assign_parameter, 0);
        assign_parameter.push_back(0x4e);
        FVM_Object *fn = make(c, assign_parameter, 1);
        if (!(fvm_function_invoke(c, fn, 0, 1, &arg, &out) == FVM_OK && out.number == (version ? 7 : 99)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes assign_argument;
        variable(assign_argument, "arguments");
        integer(assign_argument, 0);
        integer(assign_argument, 99);
        assign_argument.push_back(0x4f);
        variable(assign_argument, "x");
        fn = make(c, assign_argument, 1);
        if (!(fvm_function_invoke(c, fn, 0, 1, &arg, &out) == FVM_OK && out.number == (version ? 7 : 99)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            FVM_Value klass, proto;
            if (!(fvm_get_global(c, "Array", &klass) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_define_accessor_rooted(c, proto.object, "0", trap, trap, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *replacement = 0;
            if (!(fvm_construct(c, "Object", 0, 0, &replacement) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value v;
            fvm_value_undefined(&v);
            v.type = FVM_VALUE_OBJECT;
            v.object = replacement;
            if (!(fvm_object_set(c, klass.object, "prototype", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, klass.object, "prototype", &out) == FVM_OK && out.object == replacement))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Bytes body;
            variable(body, "arguments");
            fn = make(c, body, 0);
            char first[] = {'A', 0, 'Q'};
            FVM_Value args[2];
            if (!(fvm_value_string_bytes(args, first, 3) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(args + 1, "B");
            traps = 0;
            if (!(fvm_function_invoke(c, fn, 0, 2, args, &out) == FVM_OK && !traps))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *arguments = out.object;
            if (!(arguments->resource_kind == 28))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(c, arguments) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *actual_proto = 0;
            if (!(fvm_object_get_prototype(c, arguments, &actual_proto) == FVM_OK && actual_proto == proto.object))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            first[0] = 'Z';
            if (!(fvm_table_get_index(c, arguments, 0, &out) == FVM_OK && fvm_value_string_size(&out) == 3 &&
                  out.string[0] == 'A'))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned flags = 0;
            if (!(fvm_object_property_flags(c, arguments, "length", &flags) == FVM_OK && flags == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            int deleted = 1;
            if (!(fvm_object_delete(c, arguments, "length", &deleted) == FVM_OK && !deleted))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_call(c, arguments, "join", 0, 0, &out) == FVM_OK && fvm_value_string_size(&out) == 5 &&
                  !std::memcmp(out.string, "A\0Q,B", 5) && !traps))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_table_get_index(c, arguments, 0, &out) == FVM_OK && out.string[0] == 'A'))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_delete(c, proto.object, "0", &deleted) == FVM_OK && deleted))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Channel *channel = 0;
            if (!(fvm_channel_create(&channel) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            v.type = FVM_VALUE_OBJECT;
            v.object = arguments;
            if (!(fvm_channel_send_value(c, channel, &v, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Object *copy = out.object;
            if (!(copy->resource_kind == 28))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_pin(other, copy) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_collect(other, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_call(other, copy, "join", 0, 0, &out) == FVM_OK && fvm_value_string_size(&out) == 5 &&
                  !std::memcmp(out.string, "A\0Q,B", 5)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_channel_destroy(channel);
        }
        if (!(!c->script_callee && !c->script_depth && !c->gc_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
