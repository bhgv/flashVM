/* Ordinary accessor assignment stores latent data, preserving getter, readonly and error behavior. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static void verify(FVM_Context *c, FVM_Object *array, FVM_Object *o)
{
    FVM_Value out, value; assert(borrow(c, array, "slice", o, 0, 0, &out) == FVM_OK);
    number(c, out.object, "length", 4);
    assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
    assert(fvm_table_get_index(c, out.object, 1, &value) == FVM_OK && value.type == FVM_VALUE_OBJECT && value.object == o);
    assert(fvm_table_get_index(c, out.object, 2, &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
    assert(fvm_table_get_index(c, out.object, 3, &value) == FVM_OK && value.type == FVM_VALUE_NUMBER && value.number == 9);
    for (unsigned i = 0; i < 4; ++i) { char key[2] = {(char)('0' + i), 0}; number(c, o, key, 42); }
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0, *other = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
    const unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
    assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK);
    assert(fvm_standard_install(c) == FVM_OK); assert(fvm_standard_install(other) == FVM_OK);
    FVM_Object *array = 0, *other_array = 0, *o = 0, *get = 0, *set = 0, *fail = 0, *plain = 0;
    assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
    assert(fvm_construct(other, "Array", 0, 0, &other_array) == FVM_OK); assert(fvm_gc_pin(other, other_array) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
    assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK);
    const unsigned char getter[] = {0x96,5,0,7,42,0,0,0,0x3e,0}, setter[] = {0}, failure[] = {0x7f,0};
    assert(fvm_function_create_avm1(c, getter, sizeof(getter), 0, 0, 0, &get) == FVM_OK);
    assert(fvm_function_create_avm1(c, setter, sizeof(setter), 0, 0, 0, &set) == FVM_OK);
    assert(fvm_function_create_avm1(c, failure, sizeof(failure), 0, 0, 0, &fail) == FVM_OK);
    FVM_Value value, args[3], out; fvm_value_number(&value, 4); assert(fvm_object_set(c, o, "length", &value) == FVM_OK);
    fvm_value_number(&value, 9); assert(fvm_object_set(c, o, "3", &value) == FVM_OK);
    args[1] = object(get);
    for (unsigned i = 0; i < 4; ++i) {
        char key[2] = {(char)('0' + i), 0}; fvm_value_string(args, key);
        if (i == 2) { fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL; }
        else args[2] = object(i == 0 ? set : i == 1 ? plain : fail);
        assert(fvm_object_call(c, o, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    }
    fvm_value_string_bytes(&value, "a\0b", 3); assert(fvm_object_set(c, o, "0", &value) == FVM_OK);
    value = object(o); assert(fvm_object_set(c, o, "1", &value) == FVM_OK);
    fvm_value_number(&value, 81); assert(fvm_object_set(c, o, "2", &value) == FVM_OK);
    assert(fvm_object_set(c, o, "3", &value) == FVM_UNSUPPORTED);
    assert(c->calling_setter_owner == 0);
    assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK); verify(c, array, o);
    FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK);
    value = object(o); assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK);
    assert(fvm_channel_receive_value(other, channel, &value, 0) == FVM_OK); assert(fvm_gc_pin(other, value.object) == FVM_OK);
    assert(fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); verify(other, other_array, value.object);
    assert(c->gc_frames == 0 && c->avm1_execution_depth == 0);
    fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
