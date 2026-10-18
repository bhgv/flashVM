#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
        assert(fvm_standard_install(c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value value, out; FVM_Object *str = 0, *child = 0;
        fvm_value_string_bytes(&value, "a\0b", 3); assert(fvm_construct(c, "String", 1, &value, &str) == FVM_OK); assert(fvm_gc_pin(c, str) == FVM_OK); number(c, str, "length", 3);
        unsigned flags = 0; assert(fvm_object_property_flags(c, str, "length", &flags) == FVM_OK && flags == (version ? 3U : 0U));
        fvm_value_number(&value, 81); assert(fvm_object_set(c, str, "length", &value) == FVM_OK); number(c, str, "length", version ? 81 : 3);
        assert(fvm_object_set_property_flags(c, str, "length", 4, 0) == FVM_OK); fvm_value_number(&value, 82); assert(fvm_object_set(c, str, "length", &value) == FVM_OK); number(c, str, "length", version ? 81 : 3);
        assert(fvm_object_set_property_flags(c, str, "length", 0, 4) == FVM_OK); fvm_value_number(&value, 2);
        assert(fvm_object_call(c, str, "charAt", 1, &value, &out) == FVM_OK && !std::strcmp(out.string, "b"));
        assert(fvm_construct(c, "Object", 0, 0, &child) == FVM_OK); fvm_value_string_bytes(&value, "x\0y", 3); assert(fvm_object_set(c, child, "marker", &value) == FVM_OK);
        value = object(str); assert(fvm_object_set(c, child, "owner", &value) == FVM_OK); value = object(child); assert(fvm_object_set(c, str, "length", &value) == FVM_OK);
        assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
        if (version) { assert(fvm_object_get(c, str, "length", &value) == FVM_OK && value.object == child); int deleted = 0; assert(fvm_object_delete(c, str, "length", &deleted) == FVM_OK && !deleted); }
        else number(c, str, "length", 3);
        FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); value = object(str);
        assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK);
        assert(fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK);
        assert(fvm_object_call(other, copy, "valueOf", 0, 0, &value) == FVM_OK && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
        assert(fvm_object_property_flags(other, copy, "length", &flags) == FVM_OK && flags == (version ? 3U : 0U));
        if (version) {
            assert(fvm_object_get(other, copy, "length", &value) == FVM_OK && value.type == FVM_VALUE_OBJECT); FVM_Object *copied_child = value.object;
            assert(fvm_object_get(other, copied_child, "owner", &value) == FVM_OK && value.object == copy);
            assert(fvm_object_get(other, copied_child, "marker", &value) == FVM_OK && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "x\0y", 3));
            assert(fvm_object_set_property_flags(other, copy, "length", 0, 2) == FVM_OK); int deleted = 0; assert(fvm_object_delete(other, copy, "length", &deleted) == FVM_OK && deleted);
            int own = 1; assert(fvm_object_has_own(other, copy, "length", &own) == FVM_OK && !own);
        } else number(other, copy, "length", 3);
        assert(c->gc_frames == 0 && other->gc_frames == 0 && !c->avm1_execution_depth && !other->avm1_execution_depth);
        fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
