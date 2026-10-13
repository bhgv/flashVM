/* Loaded indexed assignment grows length before data flags and retains readonly outside slots. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include "flashvm/serialization.h"
#include "zlib.h"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
        if (version) {
            unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK);
        }
        assert(fvm_standard_install(c) == FVM_OK); assert(fvm_standard_install(other) == FVM_OK);
        FVM_Value value, out, args[3]; FVM_Object *array = 0, *child = 0;
        fvm_value_string(&value, "first"); assert(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_object_set_property_flags(c, array, "length", 4, 0) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &child) == FVM_OK); fvm_value_string_bytes(&value, "a\0b", 3); assert(fvm_object_set(c, child, "marker", &value) == FVM_OK);
        value = object(child); assert(fvm_table_set_index(c, array, 3, &value) == FVM_OK);
        int own = 0; assert(fvm_object_has_own(c, array, "3", &own) == FVM_OK && (own != 0) == (version != 0)); number(c, array, "length", 1);
        assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
        if (version) {
            assert(fvm_table_get_index(c, array, 3, &value) == FVM_OK && value.type == FVM_VALUE_OBJECT);
            assert(fvm_object_get(c, value.object, "marker", &value) == FVM_OK && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
            FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); value = object(array);
            assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); assert(fvm_gc_pin(other, out.object) == FVM_OK);
            assert(fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); number(other, out.object, "length", 1);
            assert(fvm_table_get_index(other, out.object, 3, &value) == FVM_OK && value.type == FVM_VALUE_OBJECT);
            assert(fvm_object_get(other, value.object, "marker", &value) == FVM_OK && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3)); fvm_channel_destroy(channel);
            for (unsigned fixed = 0; fixed < 2; ++fixed) {
                fvm_value_string(&value, "first"); assert(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK);
                FVM_Object *get_fn = 0, *set_fn = 0;
                const unsigned char getter_code[] = {0x96,5,0,7,42,0,0,0,0x3e,0}, setter_code[] = {0x96,6,0,0,'t','h','i','s',0,0x1c,0x96,13,0,0,'c','a','l','l','e','d',0,7,1,0,0,0,0x4f,0};
                assert(fvm_function_create_avm1(c, getter_code, sizeof(getter_code), 0, 0, 0, &get_fn) == FVM_OK); assert(fvm_function_create_avm1(c, setter_code, sizeof(setter_code), 0, 0, 0, &set_fn) == FVM_OK);
                fvm_value_string(args, "3"); args[1] = object(get_fn); args[2] = object(set_fn); assert(fvm_object_call(c, array, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
                assert(fvm_object_set_property_flags(c, array, "3", 4, 0) == FVM_OK);
                if (fixed) assert(fvm_object_set_property_flags(c, array, "length", 4, 0) == FVM_OK);
                fvm_value_number(&value, 81); assert(fvm_table_set_index(c, array, 3, &value) == FVM_OK); number(c, array, "called", 1); number(c, array, "length", fixed ? 1 : 4);
                if (fixed) { assert(fvm_object_set_property_flags(c, array, "length", 0, 4) == FVM_OK); fvm_value_number(&value, 4); assert(fvm_object_set(c, array, "length", &value) == FVM_OK); }
                assert(fvm_object_call(c, array, "slice", 0, 0, &out) == FVM_OK); assert(fvm_table_get_index(c, out.object, 3, &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
                assert(c->gc_frames == 0 && c->calling_setter_owner == 0 && c->calling_setter_name == 0);
            }
        }
        /* A bare one-record Array graph keeps the reserved-index boundary independently testable. */
        fvm_value_string(&value, "first"); assert(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK);
        array->prototype = 0; array->accessors.clear();
        unsigned char *wire = 0; size_t wire_size = 0; value = object(array);
        assert(fvm_value_serialize(c, &value, &wire, &wire_size) == FVM_OK);
        assert(wire_size > 70 && wire[28] == 1 && wire[33] == FVM_VALUE_NUMBER && wire[65] == 1);
        std::memset(wire + 66, 0xff, 4);
        unsigned long checksum = (unsigned long)crc32(0, wire + 16, (uInt)(wire_size - 16));
        for (unsigned i = 0; i < 4; ++i) wire[12 + i] = (unsigned char)(checksum >> (24 - i * 8));
        unsigned long before = 0, after = 0; assert(fvm_gc_object_count(other, &before) == FVM_OK);
        assert(fvm_value_deserialize(other, wire, wire_size, &out) == FVM_INVALID_ARGUMENT && out.type == FVM_VALUE_UNDEFINED);
        assert(fvm_gc_object_count(other, &after) == FVM_OK && before == after); fvm_serialized_free(wire);
        fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
