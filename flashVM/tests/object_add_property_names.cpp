#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/transfer.h"
#include <cassert>
#include <cstring>
static unsigned int reads;
static FVM_Result getter(FVM_Context *, FVM_Object *, unsigned int, const FVM_Value *,
                         FVM_Value *out, void *)
{
    ++reads;
    fvm_value_number(out, 4);
    return FVM_OK;
}
static FVM_Value object(FVM_Object *o)
{
    FVM_Value v;
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = o;
    return v;
}
static FVM_Result borrow(FVM_Context *c, FVM_Object *array, const char *name, FVM_Object *receiver,
                         unsigned int n, const FVM_Value *args, FVM_Value *out)
{
    FVM_Value method, call[8];
    assert(n < 7);
    assert(fvm_object_get(c, array, name, &method) == FVM_OK && method.type == FVM_VALUE_OBJECT);
    call[0] = object(receiver);
    for (unsigned int i = 0; i < n; ++i)
        call[i + 1] = args[i];
    return fvm_object_call(c, method.object, "call", n + 1, call, out);
}
static void number(FVM_Context *c, FVM_Object *o, const char *name, double expected)
{
    FVM_Value v;
    assert(fvm_object_get(c, o, name, &v) == FVM_OK && v.type == FVM_VALUE_NUMBER &&
           v.number == expected);
}
static unsigned names, writes;
static FVM_Result name_string(FVM_Context *, FVM_Object *, unsigned int,
                              const FVM_Value *, FVM_Value *out, void *)
{
    ++names;
    return fvm_value_string(out, "coerced");
}
static FVM_Result fail_name(FVM_Context *, FVM_Object *, unsigned int,
                            const FVM_Value *, FVM_Value *, void *) { return FVM_OUT_OF_MEMORY; }
static FVM_Result setter(FVM_Context *, FVM_Object *, unsigned int,
                         const FVM_Value *, FVM_Value *out, void *)
{ ++writes; fvm_value_undefined(out); return FVM_OK; }

#ifndef FVM_OBJECT_NAMES_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    FVM_Object *o = 0, *get = 0, *set = 0, *name = 0, *fn = 0;
    FVM_Value args[3], out, value;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
    assert(fvm_function_create_native(c, getter, 0, 0, &get) == FVM_OK);
    assert(fvm_function_create_native(c, setter, 0, 0, &set) == FVM_OK);
    fvm_value_number(args, 2); args[1] = object(get);
    fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
    assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    number(c, o, "2", 4);
    unsigned flags = 0;
    assert(fvm_object_property_flags(c, o, "2", &flags) == FVM_OK && (flags & FVM_PROPERTY_READ_ONLY));
    args[2] = object(set);
    assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_property_flags(c, o, "2", &flags) == FVM_OK && (flags & FVM_PROPERTY_READ_ONLY));
    assert(fvm_object_set_property_flags(c, o, "2", 0, FVM_PROPERTY_READ_ONLY) == FVM_OK);
    fvm_value_number(&value, 8);
    assert(fvm_object_set(c, o, "2", &value) == FVM_OK && writes == 1);
    assert(fvm_value_string_bytes(args, "n\0m", 3) == FVM_OK);
    assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_get_bytes(c, o, "n\0m", 3, &out) == FVM_OK && out.number == 4);
    assert(fvm_object_get(c, o, "n", &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
    assert(fvm_construct(c, "Object", 0, 0, &name) == FVM_OK);
    assert(fvm_function_create_native(c, name_string, 0, 0, &fn) == FVM_OK);
    value = object(fn); assert(fvm_object_set(c, name, "toString", &value) == FVM_OK);
    args[0] = object(name);
    assert(fvm_object_call(c, o, "addProperty", 1, args, &out) == FVM_OK && !out.boolean && names == 1);
    assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean && names == 2);
    number(c, o, "coerced", 4);
    assert(fvm_function_create_native(c, fail_name, 0, 0, &fn) == FVM_OK);
    value = object(fn); assert(fvm_object_set(c, name, "toString", &value) == FVM_OK);
    assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OUT_OF_MEMORY);
    number(c, o, "coerced", 4);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}

#endif
