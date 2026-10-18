/* Loaded addProperty retains native Array index data and flags through GC/graph transfer. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static void verify(FVM_Context *c, FVM_Object *array, bool virtual_indices)
{
    FVM_Value out, value;
    if (virtual_indices) {
        int removed = 0; assert(fvm_object_delete(c, array, "called", &removed) == FVM_OK && removed);
        fvm_value_number(&value, 70); assert(fvm_table_set_index(c, array, 0, &value) == FVM_OK);
        number(c, array, "called", 1);
        assert(fvm_object_get(c, array, "watchedOld", &value) == FVM_OK && value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
    }
    assert(fvm_object_call(c, array, "slice", 0, 0, &out) == FVM_OK);
    number(c, out.object, "length", 3);
    assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
    number(c, out.object, "1", virtual_indices ? 81 : 9);
    assert(fvm_table_get_index(c, out.object, 2, &value) == FVM_OK && value.type == FVM_VALUE_OBJECT && value.object == array);
    unsigned flags = 0;
    assert(fvm_object_property_flags(c, array, "0", &flags) == FVM_OK && flags == 4);
    assert(fvm_object_property_flags(c, array, "1", &flags) == FVM_OK && flags == 2);
    if (virtual_indices) for (unsigned i = 0; i < 3; ++i) { char key[2] = {(char)('0' + i),0}; number(c, array, key, 42); }
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0, *other = 0;
        assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
        if (version) {
            unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK);
        }
        assert(fvm_standard_install(c) == FVM_OK); assert(fvm_standard_install(other) == FVM_OK);
        FVM_Value values[3], args[3], out; FVM_Object *array = 0, *get_fn = 0, *set_fn = 0;
        fvm_value_string_bytes(values, "a\0b", 3); fvm_value_number(values + 1, 9); fvm_value_undefined(values + 2);
        assert(fvm_construct(c, "Array", 3, values, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
        out = object(array); assert(fvm_table_set_index(c, array, 2, &out) == FVM_OK);
        assert(fvm_object_set_property_flags(c, array, "0", 4, 0) == FVM_OK); assert(fvm_object_set_property_flags(c, array, "1", 2, 0) == FVM_OK);
        const unsigned char getter_code[] = {0x96,5,0,7,42,0,0,0,0x3e,0}, setter_code[] = {0x96,6,0,0,'t','h','i','s',0,0x1c,
            0x96,13,0,0,'c','a','l','l','e','d',0,7,1,0,0,0,0x4f,0};
        assert(fvm_function_create_avm1(c, getter_code, sizeof(getter_code), 0, 0, 0, &get_fn) == FVM_OK);
        assert(fvm_function_create_avm1(c, setter_code, sizeof(setter_code), 0, 0, 0, &set_fn) == FVM_OK);
        args[1] = object(get_fn); args[2] = object(set_fn);
        for (unsigned i = 0; i < 3; ++i) {
            char key[2] = {(char)('0' + i),0}; fvm_value_string(args, key);
            assert(fvm_object_call(c, array, "addProperty", 3, args, &out) == FVM_OK && (out.boolean != 0) == (version != 0));
        }
        if (version) {
            const unsigned char watch_code[] = {0x96,6,0,0,'t','h','i','s',0,0x1c,
                0x96,17,0,0,'w','a','t','c','h','e','d','O','l','d',0,0,'o','l','d',0,0x1c,0x4f,
                0x96,5,0,7,82,0,0,0,0x3e,0};
            const char *parameters[] = {"name","old","new","userData"}; FVM_Object *watch_fn = 0;
            assert(fvm_function_create_avm1(c, watch_code, sizeof(watch_code), 4, parameters, 0, &watch_fn) == FVM_OK);
            FVM_Value watch_args[2]; fvm_value_string(watch_args, "0"); watch_args[1] = object(watch_fn);
            assert(fvm_object_call(c, array, "watch", 2, watch_args, &out) == FVM_OK && out.boolean);
            fvm_value_number(&out, 81); assert(fvm_table_set_index(c, array, 0, &out) == FVM_OK);
            number(c, array, "called", 1);
            int removed = 0; assert(fvm_object_delete(c, array, "called", &removed) == FVM_OK && removed);
            assert(fvm_table_set_index(c, array, 1, &out) == FVM_OK); number(c, array, "called", 1);
            assert(c->calling_setter_owner == 0 && c->calling_setter_name == 0);
        }
        assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK); verify(c, array, version != 0);
        FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK);
        out = object(array); assert(fvm_channel_send_value(c, channel, &out, 0) == FVM_OK);
        assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); assert(fvm_gc_pin(other, out.object) == FVM_OK);
        assert(fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); verify(other, out.object, version != 0);
        assert(c->gc_frames == 0 && other->gc_frames == 0);
        fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
