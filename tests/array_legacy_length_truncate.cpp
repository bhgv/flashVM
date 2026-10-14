/* Loaded length truncation deletes tails before readonly storage and retains nondelete outside slots. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value values[4], value; fvm_value_string(values, "first"); fvm_value_number(values + 1, 81); fvm_value_number(values + 2, 82); fvm_value_string_bytes(values + 3, "a\0b", 3);
        for (unsigned fixed = 0; fixed < 2; ++fixed) {
            FVM_Object *array = 0; assert(fvm_construct(c, "Array", 4, values, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK);
            assert(fvm_object_set_property_flags(c, array, "3", 2, 0) == FVM_OK);
            if (fixed) assert(fvm_object_set_property_flags(c, array, "length", 4, 0) == FVM_OK);
            fvm_value_number(&value, 1); assert(fvm_object_set(c, array, "length", &value) == FVM_OK);
            number(c, array, "length", version && !fixed ? 1 : 4);
            assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
            for (unsigned i = 1; i < 3; ++i) { char key[2] = {(char)('0' + i),0}; int own = 0; assert(fvm_object_has_own(c, array, key, &own) == FVM_OK && (own != 0) == (version == 0)); }
            assert(fvm_table_get_index(c, array, 3, &value) == FVM_OK && value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
            if (version && !fixed) {
                assert(fvm_object_set_property_flags(c, array, "3", 0, 2) == FVM_OK);
                fvm_value_number(&value, 0); assert(fvm_object_set(c, array, "length", &value) == FVM_OK); number(c, array, "length", 0);
                FVM_Context *other = 0; assert(fvm_context_create(rt, &other) == FVM_OK);
                FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); value = object(array);
                assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &value, 0) == FVM_OK);
                FVM_Object *copy = value.object; assert(fvm_gc_pin(other, copy) == FVM_OK);
                assert(fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); number(other, copy, "length", 0);
                assert(fvm_table_get_index(other, copy, 3, &value) == FVM_OK && value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
                fvm_channel_destroy(channel); fvm_context_destroy(other);
            }
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0 && c->calling_setter_owner == 0 && c->calling_setter_name == 0);
        }
        if (version) {
            FVM_Object *wide = 0; assert(fvm_construct(c, "Array", 4, values, &wide) == FVM_OK);
            fvm_value_number(&value, 2147483648.0); assert(fvm_object_set(c, wide, "length", &value) == FVM_OK);
            int retained = 0; assert(fvm_object_has_own(c, wide, "0", &retained) == FVM_OK && !retained);
            fvm_value_number(&value, 2147483647.0); assert(fvm_object_set(c, wide, "length", &value) == FVM_OK); number(c, wide, "length", 2147483647.0);
            FVM_Object *array = 0; assert(fvm_construct(c, "Array", 4, values, &array) == FVM_OK);
            unsigned long previous = rt->config.instruction_limit; rt->config.instruction_limit = 1;
            fvm_value_number(&value, 0); assert(fvm_object_set(c, array, "length", &value) == FVM_ERROR);
            rt->config.instruction_limit = previous; number(c, array, "length", 4);
            int own = 0; assert(fvm_object_has_own(c, array, "0", &own) == FVM_OK && !own); assert(fvm_object_has_own(c, array, "1", &own) == FVM_OK && own);
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0 && c->calling_setter_owner == 0 && c->calling_setter_name == 0);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
