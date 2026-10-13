/* Loaded readonly setters still execute while data respects the current own flags. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "../src/standard/base_state.hpp"
static unsigned calls;
static bool unlock;
static FVM_Result write(FVM_Context *c, FVM_Object *o, unsigned n, const FVM_Value *, FVM_Value *out, void *)
{
    assert(n == 1 && c->calling_setter_owner == o && c->calling_setter_name);
    if (c->calling_setter_name->size() == 3) assert(!std::memcmp(c->calling_setter_name->data(), "n\0m", 3));
    ++calls;
    if (unlock) assert(fvm_object_set_property_flags_bytes(c, o, c->calling_setter_name->data(),
                                                         c->calling_setter_name->size(), 0, 4) == FVM_OK);
    fvm_value_undefined(out); return FVM_OK;
}
static void data(FVM_Object *o, const char *key, double expected)
{
    fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator a = o->accessors.find(key);
    assert(a != o->accessors.end()); FVM_Value value;
    assert(fvm_base_accessor_data(a->second, value) && value.type == FVM_VALUE_NUMBER && value.number == expected);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 6) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) {
            unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *o = 0, *get = 0, *set = 0, *parent = 0, *child = 0;
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
        assert(fvm_function_create_native(c, getter, 0, 0, &get) == FVM_OK);
        assert(fvm_function_create_native(c, write, 0, 0, &set) == FVM_OK);
        FVM_Value value, args[3], out;
        fvm_value_number(&value, 9); assert(fvm_object_set(c, o, "0", &value) == FVM_OK);
        assert(fvm_object_set_property_flags(c, o, "0", 4, 0) == FVM_OK);
        fvm_value_string(args, "0"); args[1] = object(get); args[2] = object(set);
        assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        fvm_value_number(&value, 81); calls = 0; unlock = false;
        assert(fvm_object_set(c, o, "0", &value) == FVM_OK && calls == (version ? 1U : 0U)); data(o, "0", 9);
        if (version) { unlock = true; assert(fvm_object_set(c, o, "0", &value) == FVM_OK && calls == 2); data(o, "0", 81); }
        fvm_value_number(&value, 9); assert(fvm_object_set_bytes(c, o, "n\0m", 3, &value) == FVM_OK);
        assert(fvm_object_set_property_flags_bytes(c, o, "n\0m", 3, 4, 0) == FVM_OK);
        fvm_value_string_bytes(args, "n\0m", 3);
        assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        calls = 0; unlock = false; fvm_value_number(&value, 81);
        assert(fvm_object_set_bytes(c, o, "n\0m", 3, &value) == FVM_OK && calls == (version ? 1U : 0U));
        fvm_detail::map<fvm_detail::string, FVM_AccessorEntry>::iterator counted = o->accessors.find(fvm_detail::string("n\0m", 3));
        assert(counted != o->accessors.end() && fvm_base_accessor_data(counted->second, out) && out.number == 9);
        assert(c->calling_setter_owner == 0 && c->calling_setter_name == 0 && c->gc_frames == 0);
        assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK); assert(fvm_construct(c, "Object", 0, 0, &child) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set(c, parent, "locked", &value) == FVM_OK);
        assert(fvm_object_set_property_flags(c, parent, "locked", 4, 0) == FVM_OK);
        assert(fvm_object_set_prototype(c, child, parent) == FVM_OK);
        fvm_value_number(&value, 81); assert(fvm_object_set(c, child, "locked", &value) == FVM_OK);
        assert(fvm_object_get(c, child, "locked", &out) == FVM_OK && out.number == (version ? 81 : 9));
        assert(fvm_object_get(c, parent, "locked", &out) == FVM_OK && out.number == 9);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
