#include "flashvm/standard.h"
#include <assert.h>
#include <stdlib.h>
static void getter(FVM_Context *c, FVM_Object *date, const char *name, double expected)
{
    FVM_Value out;
    assert(fvm_object_call(c, date, name, 0, 0, &out) == FVM_OK);
    assert(out.number == expected);
}
int main(int argc, char **argv)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *date = 0;
    FVM_Value args[3], out;
    double winter_offset, summer_offset;
    if (argc == 1)
        return 0; /* Explicit TZ profiles are run by the independent driver. */
    assert(argc == 3);
    winter_offset = atof(argv[1]);
    summer_offset = atof(argv[2]);
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK);
    fvm_value_number(args, 1577880000000.0); /* 2020-01-01 12:00 UTC */
    assert(fvm_construct(c, "Date", 1, args, &date) == FVM_OK);
    getter(c, date, "getTimezoneOffset", winter_offset);
    getter(c, date, "getHours", 12 - winter_offset / 60);
    fvm_value_number(args, 1593604800000.0); /* 2020-07-01 12:00 UTC */
    assert(fvm_object_call(c, date, "setTime", 1, args, &out) == FVM_OK);
    getter(c, date, "getTimezoneOffset", summer_offset);
    getter(c, date, "getHours", 12 - summer_offset / 60);
    fvm_value_number(args, 2020);
    fvm_value_number(args + 1, 0);
    fvm_value_number(args + 2, 1);
    assert(fvm_construct(c, "Date", 3, args, &date) == FVM_OK);
    getter(c, date, "getTime", 1577836800000.0 + winter_offset * 60000);
    getter(c, date, "getHours", 0);
    fvm_value_number(args, 6);
    assert(fvm_object_call(c, date, "setMonth", 1, args, &out) == FVM_OK);
    getter(c, date, "getTime", 1593561600000.0 + summer_offset * 60000);
    getter(c, date, "getHours", 0);
    assert(fvm_construct(c, "Date", 0, 0, &date) == FVM_OK);
    assert(fvm_object_call(c, date, "getTime", 0, 0, &out) == FVM_OK);
    assert(out.number > 946684800000.0 && out.number < 4102444800000.0);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
