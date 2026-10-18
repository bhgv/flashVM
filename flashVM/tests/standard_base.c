#include "flashvm/standard.h"
#include "flashvm/table.h"
#include "flashvm/property.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <string.h>
static void length(FVM_Context *c, FVM_Object *a, double expected)
{
    FVM_Value v;
    assert(fvm_object_get(c, a, "length", &v) == FVM_OK && v.number == expected);
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *o = 0, *a = 0;
    FVM_Value v, args[3], out, copy, klass;
    FVM_Channel *channel = 0;
    int yes;
    unsigned int flags;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
    fvm_value_number(&v, 123);
    assert(fvm_object_set(c, o, "value", &v) == FVM_OK);
    fvm_value_string(args, "value");
    assert(fvm_object_call(c, o, "hasOwnProperty", 1, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_call(c, o, "isPropertyEnumerable", 1, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_set_property_flags(c, o, "value", 7, 0) == FVM_OK);
    fvm_value_number(&v, 456);
    assert(fvm_object_set(c, o, "value", &v) == FVM_OK);
    assert(fvm_object_get(c, o, "value", &out) == FVM_OK && out.number == 123);
    assert(fvm_object_delete(c, o, "value", &yes) == FVM_OK && !yes);
    assert(fvm_object_call(c, o, "propertyIsEnumerable", 1, args, &out) == FVM_OK && !out.boolean);
    assert(fvm_object_set_property_flags(c, o, "value", 0, 7) == FVM_OK);
    assert(fvm_object_delete(c, o, "value", &yes) == FVM_OK && yes);
    fvm_value_number(args, 4);
    assert(fvm_construct(c, "Array", 1, args, &a) == FVM_OK);
    length(c, a, 4);
    fvm_value_string(&v, "four");
    assert(fvm_object_set(c, a, "4", &v) == FVM_OK);
    length(c, a, 5);
    assert(fvm_table_get_index(c, a, 4, &out) == FVM_OK && !strcmp(out.string, "four"));
    fvm_value_string(&v, "leading");
    assert(fvm_object_set(c, a, "04", &v) == FVM_OK);
    length(c, a, 5);
    assert(fvm_object_get(c, a, "04", &out) == FVM_OK && !strcmp(out.string, "leading"));
    fvm_value_string(args, "!");
    assert(fvm_object_call(c, a, "push", 1, args, &out) == FVM_OK && out.number == 6);
    assert(fvm_object_call(c, a, "join", 0, 0, &out) == FVM_OK &&
           !strcmp(out.string, ",,,,four,!"));
    assert(fvm_object_call(c, a, "reverse", 0, 0, &out) == FVM_OK);
    length(c, a, 6);
    assert(fvm_object_call(c, a, "shift", 0, 0, &out) == FVM_OK && !strcmp(out.string, "!"));
    assert(fvm_object_call(c, a, "pop", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    length(c, a, 4);
    fvm_value_number(args, 1);
    fvm_value_number(args + 1, 2);
    fvm_value_string(args + 2, "insert");
    assert(fvm_object_call(c, a, "splice", 3, args, &out) == FVM_OK &&
           out.type == FVM_VALUE_OBJECT);
    length(c, a, 3);
    assert(fvm_object_call(c, a, "join", 0, 0, &out) == FVM_OK &&
           !strcmp(out.string, "four,insert,"));
    fvm_value_number(args, 1);
    assert(fvm_object_set(c, a, "length", args) == FVM_OK);
    length(c, a, 1);
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = a;
    assert(fvm_object_set(c, a, "cycle", &v) == FVM_OK);
    assert(fvm_table_set_index(c, a, 1, &v) == FVM_OK);
    assert(fvm_object_call(c, a, "join", 0, 0, &out) == FVM_OK && !strcmp(out.string, "four,"));
    assert(fvm_object_set_property_flags(c, a, "cycle", 3, 0) == FVM_OK);
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_channel_send_value(c, channel, &v, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
    assert(fvm_gc_pin(other, copy.object) == FVM_OK);
    assert(fvm_gc_collect(other, 0) == FVM_OK);
    length(other, copy.object, 2);
    assert(fvm_object_get(other, copy.object, "cycle", &out) == FVM_OK &&
           out.object == copy.object);
    assert(fvm_object_property_flags(other, copy.object, "cycle", &flags) == FVM_OK && flags == 3);
    assert(fvm_object_call(other, copy.object, "join", 0, 0, &out) == FVM_OK &&
           !strcmp(out.string, "four,"));
    assert(fvm_get_global(c, "Object", &klass) == FVM_OK);
    assert(fvm_value_instance_of(c, &v, &klass, &yes) == FVM_OK && yes);
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
