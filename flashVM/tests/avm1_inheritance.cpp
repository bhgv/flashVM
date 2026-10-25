#include <stdio.h>
#include <stdlib.h>
#include "flashvm/avm1.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/table.h"
#include "flashvm/gc.h"
#include "flashvm/metamethod.h"
#include "../src/details/vector.hpp"
#include <cstring>

typedef fvm_detail::vector<unsigned char> Bytes;


static void text(Bytes &b, const char *s)
{
    size_t n = std::strlen(s) + 2;
    b.push_back(0x96);
    b.push_back((unsigned char)n);
    b.push_back((unsigned char)(n >> 8));
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


static void integer(Bytes &b, unsigned int n)
{
    b.push_back(0x96);
    b.push_back(5);
    b.push_back(0);
    b.push_back(7);
    for (unsigned int i = 0; i < 4; ++i)
    {
        b.push_back((unsigned char)n);
        n >>= 8;
    }
}


static void run(FVM_Context *c, Bytes &b)
{
    b.push_back(0);
    if (!(fvm_avm1_execute(c, b.begin(), b.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Value function(FVM_Context *c, const char *name)
{
    FVM_Object *f = 0;
    const unsigned char empty_body[] = {0};
    if (!(fvm_function_create_avm1(c, empty_body, sizeof(empty_body), 0, 0, 0, &f) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = f;
    if (!(fvm_set_global(c, name, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return v;
}


static void implements(FVM_Context *c, const char *klass, const char *iface)
{
    Bytes b;
    variable(b, iface);
    integer(b, 1);
    variable(b, klass);
    b.push_back(0x2c);
    run(c, b);
}


static void cast(FVM_Context *c, const char *type, bool success)
{
    Bytes b;
    text(b, "cast_result");
    variable(b, type);
    variable(b, "instance");
    b.push_back(0x2b);
    b.push_back(0x1d);
    run(c, b);
    FVM_Value out, instance;
    if (!(fvm_get_global(c, "cast_result", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "instance", &instance) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(success ? out.type == FVM_VALUE_OBJECT && out.object == instance.object : out.type == FVM_VALUE_NULL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Result
side_effect(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *out, void *userdata)
{
    ++*(unsigned int *)userdata;
    fvm_value_undefined(out);
    return FVM_OK;
}


struct FinalizerState
{
    FVM_Value instance, interface;
    unsigned int calls;
};


static FVM_Result
finalize(FVM_Context *c, FVM_Object *, unsigned int, const FVM_Value *, FVM_Value *out, void *userdata)
{
    FinalizerState *state = (FinalizerState *)userdata;
    int match = 0;
    FVM_Result result = fvm_value_instance_of(c, &state->instance, &state->interface, &match);
    if (!(result == FVM_OK && match))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++state->calls;
    fvm_value_undefined(out);
    return result;
}


int main()
{
    FVM_Config config;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&config, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value base = function(c, "Base"), sub = function(c, "Sub");
    FVM_Value iface = function(c, "Iface"), parent_iface = function(c, "ParentIface");
    FVM_Value unrelated = function(c, "Other"), proto, value;
    if (!(fvm_object_get(c, base.object, "prototype", &proto) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 42);
    if (!(fvm_object_set(c, proto.object, "inherited", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned int native_calls = 0;
    FVM_Object *native_ctor = 0, *native_proto = 0;
    if (!(fvm_function_create_native(c, side_effect, &native_calls, 0, &native_ctor) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &native_proto) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value native;
    fvm_value_undefined(&native);
    native.type = FVM_VALUE_OBJECT;
    native.object = native_proto;
    if (!(fvm_object_set(c, native_ctor, "prototype", &native) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    native.object = native_ctor;
    if (!(fvm_set_global(c, "Native", &native) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes native_extend;
    variable(native_extend, "Sub");
    variable(native_extend, "Native");
    native_extend.push_back(0x69);
    run(c, native_extend);
    if (!(native_calls == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes extend;
    variable(extend, "Sub");
    variable(extend, "Base");
    extend.push_back(0x69);
    run(c, extend);
    if (!(fvm_object_get(c, sub.object, "prototype", &proto) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, proto.object, "constructor", &value) == FVM_OK && value.object == base.object))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *instance = 0;
    if (!(fvm_function_construct(c, sub.object, 0, 0, &instance) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = instance;
    if (!(fvm_set_global(c, "instance", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, instance, "inherited", &proto) == FVM_OK && proto.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int match = 0;
    if (!(fvm_value_instance_of(c, &value, &base, &match) == FVM_OK && match))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_instance_of(c, &value, &sub, &match) == FVM_OK && match))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    implements(c, "Iface", "ParentIface");
    implements(c, "ParentIface", "Iface");
    implements(c, "Base", "Iface");
    if (!(fvm_value_instance_of(c, &value, &iface, &match) == FVM_OK && match))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_instance_of(c, &value, &parent_iface, &match) == FVM_OK && match))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_instance_of(c, &value, &unrelated, &match) == FVM_OK && !match))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cast(c, "Iface", true);
    cast(c, "Other", false);
    FVM_Value absent;
    fvm_value_undefined(&absent);
    if (!(fvm_set_global(c, "Iface", &absent) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_set_global(c, "ParentIface", &absent) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_instance_of(c, &value, &parent_iface, &match) == FVM_OK && match))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        FinalizerState state;
        state.instance = value;
        state.interface = parent_iface;
        state.calls = 0;
        FVM_Object *meta = 0, *garbage = 0;
        if (!(fvm_object_create(c, &meta) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &garbage) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_meta_register_rooted(c, meta, "__gc", finalize, &state, instance) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value dead;
        fvm_value_undefined(&dead);
        dead.type = FVM_VALUE_OBJECT;
        dead.object = garbage;
        if (!(fvm_value_set_metatable(c, &dead, meta) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK && state.calls == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_Object *bundle = 0;
    if (!(fvm_object_create(c, &bundle) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, bundle, "instance", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, bundle, "iface", &iface) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, bundle, "parent", &parent_iface) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, bundle, "other", &unrelated) == FVM_OK))
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
    value.object = bundle;
    if (!(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, channel, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value copied, copied_iface, copied_other;
    if (!(fvm_object_get(other, value.object, "instance", &copied) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(other, value.object, "parent", &copied_iface) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(other, value.object, "other", &copied_other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_instance_of(other, &copied, &copied_iface, &match) == FVM_OK && match))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_instance_of(other, &copied, &copied_other, &match) == FVM_OK && !match))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
