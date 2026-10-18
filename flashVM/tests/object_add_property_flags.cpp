/* Virtual replacement preserves existing flags, including protected data and writable null setters. */
#define FVM_OBJECT_NAMES_FIXTURE_ONLY
#include "object_add_property_names.cpp"
#include "flashvm/gc.h"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
    FVM_Object *o = 0, *plain = 0, *fn = 0; FVM_Value args[3], value, out; unsigned flags = 0;
    assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK); assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK);
    fvm_value_number(&value, 9); assert(fvm_object_set(c, o, "protected", &value) == FVM_OK);
    assert(fvm_object_set_property_flags(c, o, "protected", 3, 0) == FVM_OK);
    fvm_value_string(args, "protected"); args[1] = object(plain); fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
    assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_property_flags(c, o, "protected", &flags) == FVM_OK && flags == 3); number(c, o, "protected", 9);
    fvm_value_number(&value, 81); assert(fvm_object_set(c, o, "protected", &value) == FVM_OK); number(c, o, "protected", 81);
    int deleted = 1; assert(fvm_object_delete(c, o, "protected", &deleted) == FVM_OK && !deleted);
    fvm_value_string(args, "new"); assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_property_flags(c, o, "new", &flags) == FVM_OK && (flags & 4));
    assert(fvm_function_create_native(c, setter, 0, 0, &fn) == FVM_OK); args[2] = object(fn);
    assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_property_flags(c, o, "new", &flags) == FVM_OK && (flags & 4));
    fvm_value_string_bytes(args, "p\0q", 3); fvm_value_number(&value, 9);
    assert(fvm_object_set_bytes(c, o, "p\0q", 3, &value) == FVM_OK);
    assert(fvm_object_set_property_flags_bytes(c, o, "p\0q", 3, 3, 0) == FVM_OK);
    args[2].type = FVM_VALUE_NULL;
    assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_property_flags_bytes(c, o, "p\0q", 3, &flags) == FVM_OK && flags == 3);
    assert(fvm_object_get_bytes(c, o, "p\0q", 3, &out) == FVM_OK && out.number == 9);
    assert(fvm_gc_collect(c, 0) == FVM_OK); number(c, o, "protected", 81);
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
