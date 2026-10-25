#include <stdio.h>
#include <stdlib.h>
#include "flashvm/standard.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include <string.h>


static void get(FVM_Context *c, FVM_Object *o, const char *name, double expected)
{
    FVM_Value v;
    if (!(fvm_object_call(c, o, name, 0, 0, &v) == FVM_OK && v.number == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(c, "Date", &klass) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 0);
    if (!(fvm_construct(c, "Date", 1, &value, &date) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    get(c, date, "getUTCFullYear", 1970);
    get(c, date, "getUTCMonth", 0);
    get(c, date, "getUTCDate", 1);
    get(c, date, "getUTCDay", 4);
    fvm_value_number(args, 2000);
    fvm_value_number(args + 1, 1);
    fvm_value_number(args + 2, 29);
    if (!(fvm_object_call(c, klass.object, "UTC", 3, args, &out) == FVM_OK && out.number == 951782400000.0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, date, "setTime", 1, &out, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    get(c, date, "getUTCFullYear", 2000);
    get(c, date, "getUTCYear", 100);
    get(c, date, "getUTCDate", 29);
    fvm_value_number(args, 2001);
    if (!(fvm_object_call(c, date, "setUTCFullYear", 1, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    get(c, date, "getUTCMonth", 2);
    get(c, date, "getUTCDate", 1);
    fvm_value_number(args, 24);
    if (!(fvm_object_call(c, date, "setUTCHours", 1, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    get(c, date, "getUTCDate", 2);
    fvm_value_number(args, -1);
    if (!(fvm_object_call(c, date, "setTime", 1, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    get(c, date, "getUTCFullYear", 1969);
    get(c, date, "getUTCDate", 31);
    get(c, date, "getUTCMilliseconds", 999);
    fvm_value_number(args, 8640000000000000.0);
    if (!(fvm_object_call(c, date, "setTime", 1, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    get(c, date, "getUTCFullYear", 275760);
    get(c, date, "getUTCMonth", 8);
    get(c, date, "getUTCDate", 13);
    fvm_value_number(args, 8640000000000001.0);
    if (!(fvm_object_call(c, date, "setTime", 1, args, &out) == FVM_OK && out.number != out.number))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, date, "toString", 0, 0, &out) == FVM_OK && !strcmp(out.string, "Invalid Date")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 2000);
    if (!(fvm_object_call(c, date, "setUTCFullYear", 1, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    get(c, date, "getUTCMonth", 0);
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = date;
    if (!(fvm_object_set(c, date, "self", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_create(&channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(other, copy.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    get(other, copy.object, "getUTCFullYear", 2000);
    if (!(fvm_object_get(other, copy.object, "constructor", &klass) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_instance_of(other, &copy, &klass, &same) == FVM_OK && same))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
