/* Builtin fields whose user attributes must survive replacing the loaded SWF. */
#ifndef FVM_TEST_BUILTIN_RELOAD_FIXTURE_HPP
#define FVM_TEST_BUILTIN_RELOAD_FIXTURE_HPP
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/property.h"
static const char *reload_classes[] = {"Object", "Array", "String", "Number", "Boolean", "Date", "Function", "Function", "Error", "Math"};
static const char *reload_fields[] = {"valueOf", "push", "charAt", "toString", "toString", "getTime", "call", "apply", "toString", "PI"};
static FVM_Object *reload_target(FVM_Context *c, unsigned index)
{
    FVM_Value klass, prototype;
    assert(fvm_get_global(c, reload_classes[index], &klass) == FVM_OK && klass.type == FVM_VALUE_OBJECT);
    if (index == 9) return klass.object;
    assert(fvm_object_get(c, klass.object, "prototype", &prototype) == FVM_OK && prototype.type == FVM_VALUE_OBJECT);
    return prototype.object;
}
static void reload_flags(FVM_Context *c, FVM_Object *target, const char *name, unsigned expected)
{ unsigned actual = 99; assert(fvm_object_property_flags(c, target, name, &actual) == FVM_OK && actual == expected); }
static void reload_load(FVM_Context *c, unsigned version)
{
    unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
    assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
}
#endif
