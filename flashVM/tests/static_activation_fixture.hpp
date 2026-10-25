/* Host class metadata must activate independently of its public prototype. */
#include <stdio.h>
#include <stdlib.h>
#ifndef FVM_TEST_STATIC_ACTIVATION_FIXTURE_HPP
#define FVM_TEST_STATIC_ACTIVATION_FIXTURE_HPP
#include "math_attribute_fixture.hpp"

static const char *static_classes[] = {"Number", "Date", "String", "Object", "Array"};
static const char *static_fields[] = {
    "MAX_VALUE", "MIN_VALUE", "NaN", "NEGATIVE_INFINITY", "POSITIVE_INFINITY", "UTC", "fromCharCode", "registerClass"};
static unsigned static_reads;


static FVM_Result static_getter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++static_reads;
    fvm_value_number(out, 23);
    return FVM_OK;
}


static FVM_Object *static_class(FVM_Context *c, unsigned type)
{
    FVM_Value value;
    if (!(fvm_get_global(c, static_classes[type], &value) == FVM_OK && value.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return value.object;
}


static void static_prototype(FVM_Context *c, FVM_Object *klass, unsigned variant)
{
    FVM_Value value;
    int removed = 0;
    if (!variant)
    {
        return;
    }
    if (variant != 6)
    {
        if (!(fvm_object_set_property_flags(c, klass, "prototype", 0, 7) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (variant == 1)
    {
        if (!(fvm_object_delete(c, klass, "prototype", &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return;
    }
    if (variant == 2)
    {
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_NULL;
    }
    if (variant == 3)
    {
        fvm_value_number(&value, 12);
    }
    if (variant == 4)
    {
        FVM_Object *object = 0;
        if (!(fvm_object_create(c, &object) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = class_value(object);
    }
    if (variant == 5)
    {
        if (!(fvm_object_delete(c, klass, "prototype", &removed) == FVM_OK && removed &&
              fvm_object_define_accessor_rooted(c, klass, "prototype", static_getter, 0, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return;
    }
    if (variant == 6)
    {
        if (!(fvm_object_get(c, klass, "prototype", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 9);
        FVM_Value proto;
        if (!(fvm_object_get(c, klass, "prototype", &proto) == FVM_OK &&
              fvm_object_set(c,
                             proto.object,
                             klass->resource_kind == FVM_RESOURCE_DATE_CLASS     ? "getTime"
                             : klass->resource_kind == FVM_RESOURCE_STRING_CLASS ? "charAt"
                                                                                 : "valueOf",
                             &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        return;
    }
    if (!(fvm_object_set(c, klass, "prototype", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void static_flags(FVM_Context *c, FVM_Object *klass, unsigned type, unsigned expected)
{
    const unsigned begin = type ? type + 4 : 0, end = type ? type + 5 : 5;
    for (unsigned i = begin; i < end; ++i)
    {
        math_flag(c, klass, static_fields[i], expected);
    }
}


static void static_load(FVM_Context *c, unsigned version)
{
    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    swf[3] = (unsigned char)version;
    if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#endif
