/* Math metadata queries shared by loaded-layout and allocation regressions. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_TEST_MATH_ATTRIBUTE_FIXTURE_HPP
#define FVM_TEST_MATH_ATTRIBUTE_FIXTURE_HPP
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/serialization.h"

static const char *math_fields[] = {"E",    "LN10", "LN2",  "LOG10E", "LOG2E", "PI",  "SQRT1_2", "SQRT2", "abs",
                                    "acos", "asin", "atan", "atan2",  "ceil",  "cos", "exp",     "floor", "log",
                                    "max",  "min",  "pow",  "random", "round", "sin", "sqrt",    "tan"};


static void math_flag(FVM_Context *c, FVM_Object *math, const char *name, unsigned expected)
{
    unsigned actual = 99;
    if (!(fvm_object_property_flags(c, math, name, &actual) == FVM_OK && actual == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void math_enumerable(FVM_Context *c, FVM_Object *math, const char *name, bool expected)
{
    FVM_Object *keys = 0;
    FVM_Value length, key;
    if (!(fvm_object_enumerate(c, math, 0, &keys) == FVM_OK && fvm_object_get(c, keys, "length", &length) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool found = false;
    for (unsigned long i = 0; i < (unsigned long)length.number; ++i)
    {
        if (!(fvm_table_get_index(c, keys, i, &key) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (key.type == FVM_VALUE_STRING && !std::strcmp(key.string, name))
        {
            found = true;
        }
    }
    if (!(found == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void math_same(const FVM_Value &a, const FVM_Value &b)
{
    if (!(a.type == b.type))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (a.type == FVM_VALUE_NUMBER)
    {
        if (!(a.number == b.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(a.type == FVM_VALUE_OBJECT && a.object == b.object))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
}
#endif
