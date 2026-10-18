/* Setter self-deletion/replacement targets the current own property without re-invoking its setter. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static unsigned mode, calls, new_calls;
static FVM_Object *get_fn, *new_set;
static FVM_Result noop(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++new_calls; fvm_value_undefined(out); return FVM_OK; }
static FVM_Result change(FVM_Context *c, FVM_Object *o, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++calls; assert(c->calling_setter_name);
    fvm_detail::string name = *c->calling_setter_name;
    if (mode == 0 || mode == 2) {
        int removed = 0; assert(fvm_object_delete_bytes(c, o, name.data(), name.size(), &removed) == FVM_OK && removed);
        if (mode == 2) {
            FVM_Value old; fvm_value_number(&old, 99); assert(fvm_object_set_bytes(c, o, name.data(), name.size(), &old) == FVM_OK);
        }
    } else {
        FVM_Value args[3], ignored; fvm_value_string_bytes(args, name.data(), name.size());
        args[1] = object(get_fn); args[2] = object(new_set);
        assert(fvm_object_call(c, o, "addProperty", 3, args, &ignored) == FVM_OK && ignored.boolean);
        if (mode == 3) assert(fvm_object_set_property_flags_bytes(c, o, name.data(), name.size(), 4, 0) == FVM_OK);
    }
    assert(*c->calling_setter_name == name);
    fvm_value_undefined(out); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    const unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
    assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
    FVM_Object *array = 0, *set = 0; assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
    assert(fvm_function_create_native(c, getter, 0, 0, &get_fn) == FVM_OK); assert(fvm_gc_pin(c, get_fn) == FVM_OK);
    assert(fvm_function_create_native(c, noop, 0, 0, &new_set) == FVM_OK); assert(fvm_gc_pin(c, new_set) == FVM_OK);
    assert(fvm_function_create_native(c, change, 0, 0, &set) == FVM_OK); assert(fvm_gc_pin(c, set) == FVM_OK);
    for (mode = 0; mode < 4; ++mode) {
        FVM_Object *o = 0; FVM_Value value, args[3], out; assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
        fvm_value_number(&value, 1); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set(c, o, "0", &value) == FVM_OK);
        fvm_value_string(args, "0"); args[1] = object(get_fn); args[2] = object(set);
        assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        fvm_value_string_bytes(&value, "a\0b", 3); calls = new_calls = 0;
        assert(fvm_object_set(c, o, "0", &value) == FVM_OK && calls == 1 && new_calls == 0);
        assert(c->calling_setter_owner == 0 && c->calling_setter_name == 0 && c->gc_frames == 0);
        assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(borrow(c, array, "slice", o, 0, 0, &out) == FVM_OK);
        assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK);
        if (mode == 3) assert(value.type == FVM_VALUE_NUMBER && value.number == 9);
        else assert(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
