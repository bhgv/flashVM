#include <stdio.h>
#include "flashvm/standard.h"
#include <stdlib.h>


static void getter(FVM_Context *c, FVM_Object *date, const char *name, double expected)
{
    FVM_Value out;
    if (!(fvm_object_call(c, date, name, 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(out.number == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
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
    {
        return 0; /* Explicit TZ profiles are run by the independent driver. */
    }
    if (!(argc == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    winter_offset = atof(argv[1]);
    summer_offset = atof(argv[2]);
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
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(args, 1577880000000.0); /* 2020-01-01 12:00 UTC */
    if (!(fvm_construct(c, "Date", 1, args, &date) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    getter(c, date, "getTimezoneOffset", winter_offset);
    getter(c, date, "getHours", 12 - winter_offset / 60);
    fvm_value_number(args, 1593604800000.0); /* 2020-07-01 12:00 UTC */
    if (!(fvm_object_call(c, date, "setTime", 1, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    getter(c, date, "getTimezoneOffset", summer_offset);
    getter(c, date, "getHours", 12 - summer_offset / 60);
    fvm_value_number(args, 2020);
    fvm_value_number(args + 1, 0);
    fvm_value_number(args + 2, 1);
    if (!(fvm_construct(c, "Date", 3, args, &date) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    getter(c, date, "getTime", 1577836800000.0 + winter_offset * 60000);
    getter(c, date, "getHours", 0);
    fvm_value_number(args, 6);
    if (!(fvm_object_call(c, date, "setMonth", 1, args, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    getter(c, date, "getTime", 1593561600000.0 + summer_offset * 60000);
    getter(c, date, "getHours", 0);
    if (!(fvm_construct(c, "Date", 0, 0, &date) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_call(c, date, "getTime", 0, 0, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(out.number > 946684800000.0 && out.number < 4102444800000.0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
