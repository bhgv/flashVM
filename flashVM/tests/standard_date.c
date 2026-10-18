#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include <assert.h>
#include <string.h>
static void get(FVM_Context *c, FVM_Object *o, const char *name, double expected)
{
    FVM_Value v;
    assert(fvm_object_call(c, o, name, 0, 0, &v) == FVM_OK && v.number == expected);
}
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *date = 0;
    FVM_Value value, klass, args[7], out, copy;
    FVM_Channel *channel = 0;
    int same;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_context_create(rt, &other) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_get_global(c, "Date", &klass) == FVM_OK);
    fvm_value_number(&value, 0);
    assert(fvm_construct(c, "Date", 1, &value, &date) == FVM_OK);
    get(c, date, "getUTCFullYear", 1970);
    get(c, date, "getUTCMonth", 0);
    get(c, date, "getUTCDate", 1);
    get(c, date, "getUTCDay", 4);
    fvm_value_number(args, 2000);
    fvm_value_number(args + 1, 1);
    fvm_value_number(args + 2, 29);
    assert(fvm_object_call(c, klass.object, "UTC", 3, args, &out) == FVM_OK &&
           out.number == 951782400000.0);
    assert(fvm_object_call(c, date, "setTime", 1, &out, &value) == FVM_OK);
    get(c, date, "getUTCFullYear", 2000);
    get(c, date, "getUTCYear", 100);
    get(c, date, "getUTCDate", 29);
    fvm_value_number(args, 2001);
    assert(fvm_object_call(c, date, "setUTCFullYear", 1, args, &out) == FVM_OK);
    get(c, date, "getUTCMonth", 2);
    get(c, date, "getUTCDate", 1);
    fvm_value_number(args, 24);
    assert(fvm_object_call(c, date, "setUTCHours", 1, args, &out) == FVM_OK);
    get(c, date, "getUTCDate", 2);
    fvm_value_number(args, -1);
    assert(fvm_object_call(c, date, "setTime", 1, args, &out) == FVM_OK);
    get(c, date, "getUTCFullYear", 1969);
    get(c, date, "getUTCDate", 31);
    get(c, date, "getUTCMilliseconds", 999);
    fvm_value_number(args, 8640000000000000.0);
    assert(fvm_object_call(c, date, "setTime", 1, args, &out) == FVM_OK);
    get(c, date, "getUTCFullYear", 275760);
    get(c, date, "getUTCMonth", 8);
    get(c, date, "getUTCDate", 13);
    fvm_value_number(args, 8640000000000001.0);
    assert(fvm_object_call(c, date, "setTime", 1, args, &out) == FVM_OK &&
           out.number != out.number);
    assert(fvm_object_call(c, date, "toString", 0, 0, &out) == FVM_OK &&
           !strcmp(out.string, "Invalid Date"));
    fvm_value_number(args, 2000);
    assert(fvm_object_call(c, date, "setUTCFullYear", 1, args, &out) == FVM_OK);
    get(c, date, "getUTCMonth", 0);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = date;
    assert(fvm_object_set(c, date, "self", &value) == FVM_OK);
    assert(fvm_channel_create(&channel) == FVM_OK);
    assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK);
    assert(fvm_gc_pin(other, copy.object) == FVM_OK);
    assert(fvm_gc_collect(other, 0) == FVM_OK);
    get(other, copy.object, "getUTCFullYear", 2000);
    assert(fvm_object_get(other, copy.object, "constructor", &klass) == FVM_OK);
    assert(fvm_value_instance_of(other, &copy, &klass, &same) == FVM_OK && same);
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
