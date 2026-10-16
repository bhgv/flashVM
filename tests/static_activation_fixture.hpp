/* Host class metadata must activate independently of its public prototype. */
#ifndef FVM_TEST_STATIC_ACTIVATION_FIXTURE_HPP
#define FVM_TEST_STATIC_ACTIVATION_FIXTURE_HPP
#include "math_attribute_fixture.hpp"
static const char *static_classes[] = {"Number", "Date"};
static const char *static_fields[] = {"MAX_VALUE", "MIN_VALUE", "NaN", "NEGATIVE_INFINITY", "POSITIVE_INFINITY", "UTC"};
static unsigned static_reads;
static FVM_Result static_getter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++static_reads; fvm_value_number(out, 23); return FVM_OK; }
static FVM_Object *static_class(FVM_Context *c, unsigned type)
{ FVM_Value value; assert(fvm_get_global(c, static_classes[type], &value) == FVM_OK && value.type == FVM_VALUE_OBJECT); return value.object; }
static void static_prototype(FVM_Context *c, FVM_Object *klass, unsigned variant)
{
    FVM_Value value; int removed = 0;
    if (!variant) return;
    if (variant != 6) assert(fvm_object_set_property_flags(c, klass, "prototype", 0, 7) == FVM_OK);
    if (variant == 1) { assert(fvm_object_delete(c, klass, "prototype", &removed) == FVM_OK && removed); return; }
    if (variant == 2) { fvm_value_undefined(&value); value.type = FVM_VALUE_NULL; }
    if (variant == 3) fvm_value_number(&value, 12);
    if (variant == 4) { FVM_Object *object = 0; assert(fvm_object_create(c, &object) == FVM_OK); value = class_value(object); }
    if (variant == 5) { assert(fvm_object_delete(c, klass, "prototype", &removed) == FVM_OK && removed && fvm_object_define_accessor_rooted(c, klass, "prototype", static_getter, 0, 0, 0) == FVM_OK); return; }
    if (variant == 6) { assert(fvm_object_get(c, klass, "prototype", &value) == FVM_OK); fvm_value_number(&value, 9); FVM_Value proto; assert(fvm_object_get(c, klass, "prototype", &proto) == FVM_OK && fvm_object_set(c, proto.object, klass->resource_kind == FVM_RESOURCE_NUMBER_CLASS ? "valueOf" : "getTime", &value) == FVM_OK); return; }
    assert(fvm_object_set(c, klass, "prototype", &value) == FVM_OK);
}
static void static_flags(FVM_Context *c, FVM_Object *klass, unsigned type, unsigned expected)
{
    const unsigned begin = type ? 5 : 0, end = type ? 6 : 5;
    for (unsigned i = begin; i < end; ++i) math_flag(c, klass, static_fields[i], expected);
}
static void static_load(FVM_Context *c, unsigned version)
{ unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
#endif
