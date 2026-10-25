/* Inherited setters affect the receiver without overwriting prototype descriptor data. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_standard_types.cpp"
#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include "../src/core/runtime.hpp"


static void property(FVM_Context *c, FVM_Object *o, const char *name, FVM_Object *get, FVM_Object *set)
{
    FVM_Value args[3], out;
    fvm_value_string(args, name);
    fvm_value_undefined(args + 1);
    args[1].type = FVM_VALUE_OBJECT;
    args[1].object = get;
    fvm_value_undefined(args + 2);
    args[2].type = set ? FVM_VALUE_OBJECT : FVM_VALUE_NULL;
    args[2].object = set;
    if (!(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void join(FVM_Context *c, FVM_Object *array, FVM_Object *o, const char *expected)
{
    FVM_Value method, args, out;
    if (!(fvm_object_get(c, array, "join", &method) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&args);
    args.type = FVM_VALUE_OBJECT;
    args.object = o;
    if (!(fvm_object_call(c, method.object, "call", 1, &args, &out) == FVM_OK && out.type == FVM_VALUE_STRING &&
          !std::strcmp(out.string, expected)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void inherited(FVM_Context *c, FVM_Object *array, FVM_Object *o, const char *parent_text)
{
    FVM_Object *parent = 0;
    if (!(fvm_object_get_prototype(c, o, &parent) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_number(&value, 99);
    if (!(fvm_object_set(c, o, "0", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&value, "receiver");
    if (!(fvm_object_set(c, o, "1", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 99);
    if (!(fvm_object_set_bytes(c, o, "n\0m", 3, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get_bytes(c, o, "n\0m", 3, &value) == FVM_OK && value.type == FVM_VALUE_NUMBER &&
          value.number == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int own = 1;
    if (!(fvm_object_has_own_bytes(c, o, "0", 1, &own) == FVM_OK && !own))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_has_own_bytes(c, o, "1", 1, &own) == FVM_OK && !own))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    join(c, array, parent, parent_text);
    join(c, array, o, "undefined,undefined,81");
    if (!(fvm_object_get(c, o, "called", &value) == FVM_OK && value.number == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->calling_setter_owner == 0 && c->gc_frames == 0 && c->avm1_execution_depth == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
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
    const unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
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
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *array = 0, *other_array = 0, *parent = 0, *o = 0, *get = 0, *set = 0, *empty = 0, *plain = 0;
    if (!(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(other, "Array", 0, 0, &other_array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(other, other_array) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char getter[] = {0x96, 5, 0, 7, 42, 0, 0, 0, 0x3e, 0}, blank[] = {0};
    if (!(fvm_function_create_avm1(c, getter, sizeof(getter), 0, 0, 0, &get) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, blank, sizeof(blank), 0, 0, 0, &empty) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes body;
    text(body, "this");
    body.push_back(0x1c);
    text(body, "2");
    integer(body, 81);
    body.push_back(0x4f);
    text(body, "this");
    body.push_back(0x1c);
    text(body, "called");
    integer(body, 1);
    body.push_back(0x4f);
    body.push_back(0);
    if (!(fvm_function_create_avm1(c, body.begin(), body.size(), 0, 0, 0, &set) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_number(&value, 2);
    if (!(fvm_object_set(c, parent, "length", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 9);
    if (!(fvm_object_set(c, parent, "0", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_string(&value, "original");
    if (!(fvm_object_set(c, parent, "1", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    property(c, parent, "0", get, set);
    property(c, parent, "1", plain, 0);
    fvm_value_number(&value, 9);
    if (!(fvm_object_set_bytes(c, parent, "n\0m", 3, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value counted[3], installed;
    fvm_value_string_bytes(counted, "n\0m", 3);
    fvm_value_undefined(counted + 1);
    counted[1].type = FVM_VALUE_OBJECT;
    counted[1].object = plain;
    fvm_value_undefined(counted + 2);
    counted[2].type = FVM_VALUE_NULL;
    if (!(fvm_object_call(c, parent, "addProperty", 3, counted, &installed) == FVM_OK && installed.boolean))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 3);
    if (!(fvm_object_set(c, o, "length", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    property(c, o, "2", get, empty);
    if (!(fvm_object_set_prototype(c, o, parent) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inherited(c, array, o, "9,original");
    fvm_value_string(&value, "changed");
    if (!(fvm_object_set(c, parent, "1", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inherited(c, array, o, "9,changed");
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inherited(c, array, o, "9,changed");
    FVM_Channel *channel = 0;
    if (!(fvm_channel_create(&channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = o;
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
    if (!(fvm_gc_pin(other, value.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    inherited(other, other_array, value.object, "9,changed");
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
