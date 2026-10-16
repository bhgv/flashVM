/* ASSetPropFlags stores flags under the selected own spelling, including NUL/Unicode names. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/property.h"
#include "flashvm/table.h"
#include "flashvm/serialization.h"
#include "../src/core/property_key.hpp"
static unsigned case_flag_getters;
static FVM_Result case_flag_getter(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ ++case_flag_getters; return FVM_ERROR; }
static void case_flag_enumeration(FVM_Context *c, FVM_Object *object, const char *stored, size_t size, unsigned expected)
{
    FVM_Object *keys = 0; FVM_Value length, key;
    assert(fvm_object_enumerate(c, object, 0, &keys) == FVM_OK && fvm_object_get(c, keys, "length", &length) == FVM_OK);
    bool found = false;
    for (unsigned long i = 0; i < (unsigned long)length.number; ++i) {
        assert(fvm_table_get_index(c, keys, i, &key) == FVM_OK);
        if (key.type == FVM_VALUE_STRING && fvm_value_string_size(&key) == size && !std::memcmp(key.string, stored, size)) found = true;
    }
    assert(found == !(expected & 1U));
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *stored_names[] = {"MiXeD", "MiXeD\0\xc3\x84", "LongPropertyNameForAllocationAndSpellingRetention_MiXeD"};
    const char *queries[] = {"mixed", "mixed\0\xc3\xa4", "longpropertynameforallocationandspellingretention_mixed"};
    const unsigned initial[] = {0, 1, 7, 4}, set[] = {2, 2, 0, 3}, clear[] = {0, 1, 7, 3};
    for (unsigned version = 0; version <= 8; ++version)
    for (unsigned type = 0; type < 3; ++type)
    for (unsigned name = 0; name < 3; ++name)
    for (unsigned route = 0; route < 5; ++route) {
        if ((version && version < 4) || (route >= 3 && !fvm_jit_available())) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *object = 0, *parent = 0; assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK);
        FVM_Value args[4], out, value, function; fvm_value_number(&value, 9);
        size_t size = name == 1 ? 8U : std::strlen(stored_names[name]);
        const char *stored = stored_names[name], *query = queries[name];
        fvm_detail::string own_key, query_key;
        assert(fvm_property_key(stored, size, own_key) == FVM_OK && fvm_property_key(query, size, query_key) == FVM_OK);
        if (!type) assert(fvm_object_set_bytes(c, object, stored, size, &value) == FVM_OK);
        if (type == 1) { FVM_Value klass, proto; assert(fvm_get_global(c, "Object", &klass) == FVM_OK && fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK); object->methods[own_key] = proto.object->methods["toString"]; }
        if (type == 2) assert(fvm_object_define_accessor_rooted_bytes(c, object, stored, size, case_flag_getter, 0, 0, 0) == FVM_OK);
        assert(fvm_object_set(c, parent, "Parent", &value) == FVM_OK && fvm_object_set_property_flags(c, parent, "Parent", 1, 0) == FVM_OK && fvm_object_set_prototype(c, object, parent) == FVM_OK);
        args[0] = class_value(object); assert(fvm_set_global(c, "caseFlagObject", args) == FVM_OK && fvm_get_global(c, "ASSetPropFlags", &function) == FVM_OK);
        fvm_detail::vector<char> list;
        for (size_t i = 0; i < size; ++i) list.push_back(query[i]); list.push_back(',');
        for (size_t i = 0; i < size; ++i) list.push_back(query[i]);
        const char suffix[] = ",parent"; for (size_t i = 0; i < sizeof(suffix) - 1; ++i) list.push_back(suffix[i]);
        assert(fvm_value_string_bytes(args + 1, list.begin(), list.size()) == FVM_OK && fvm_set_global(c, "caseFlagNames", args + 1) == FVM_OK);
        case_flag_getters = 0;
        for (unsigned phase = 0; phase < 4; ++phase) {
            assert(fvm_object_set_property_flags_bytes(c, object, stored, size, 0, 7) == FVM_OK && fvm_object_set_property_flags_bytes(c, object, stored, size, initial[phase], 0) == FVM_OK);
            fvm_value_number(args + 2, set[phase]); fvm_value_number(args + 3, clear[phase]);
            if (!route) assert(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK);
            else if (route == 1) assert(fvm_function_invoke(c, function.object, 0, 4, args, &out) == FVM_OK);
            else { class_mode = route - 2; Program code; code.number(clear[phase]); code.number(set[phase]); code.variable("caseFlagNames"); code.variable("caseFlagObject"); code.number(4); code.text("ASSetPropFlags"); code.op(0x3d); code.finish(); out = class_run(c, code); }
            unsigned expected = version ? (initial[phase] & ~clear[phase]) | set[phase] : initial[phase], actual = 99;
            assert(out.type == FVM_VALUE_UNDEFINED && fvm_object_property_flags_bytes(c, object, stored, size, &actual) == FVM_OK && actual == expected);
            assert(object->attributes.find(query_key) == object->attributes.end());
            assert(expected ? object->attributes.find(own_key) != object->attributes.end() && object->attributes.find(own_key)->second == expected : object->attributes.find(own_key) == object->attributes.end());
            assert(fvm_object_property_flags(c, parent, "Parent", &actual) == FVM_OK && actual == 1 && object->attributes.find("parent") == object->attributes.end());
            case_flag_enumeration(c, object, stored, size, expected); assert(!case_flag_getters);
            if (type != 2) {
                FVM_Context *other = 0; assert(fvm_context_create(rt, &other) == FVM_OK);
                const unsigned char swf8[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
                unsigned char *data = 0; size_t bytes = 0;
                assert(fvm_load_memory(other, swf8, sizeof(swf8)) == FVM_OK && fvm_value_serialize(c, args, &data, &bytes) == FVM_OK && fvm_value_deserialize(other, data, bytes, &value) == FVM_OK);
                fvm_serialized_free(data); assert(fvm_gc_pin(other, value.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK && fvm_object_property_flags_bytes(other, value.object, stored, size, &actual) == FVM_OK && actual == expected);
                case_flag_enumeration(other, value.object, stored, size, expected);
                fvm_context_destroy(other);
            }
        }
        assert(!c->gc_frames && !c->gc_native_depth && !case_flag_getters);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
