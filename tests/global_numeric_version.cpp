#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include <cmath>
static void check(FVM_Context *c, FVM_Object *o, unsigned version)
{
    FVM_Value value; assert(fvm_object_get(c, o, "NaN", &value) == FVM_OK);
    assert(version == 4 ? value.type == FVM_VALUE_UNDEFINED : value.type == FVM_VALUE_NUMBER && value.number != value.number);
    assert(fvm_object_get(c, o, "Infinity", &value) == FVM_OK);
    assert(version == 4 ? value.type == FVM_VALUE_UNDEFINED : value.type == FVM_VALUE_NUMBER && value.number == HUGE_VAL);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        for (unsigned before = 0; before < 2; ++before) {
            FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
            unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)(version ? version : 4);
            if (before) assert(fvm_standard_install(c) == FVM_OK);
            if (version) assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
            assert(fvm_standard_install(c) == FVM_OK); check(c, c->global, version);
            FVM_Value value; FVM_Object *child = 0; assert(fvm_construct(c, "Object", 0, 0, &child) == FVM_OK);
            fvm_value_string_bytes(&value, "a\0b", 3); assert(fvm_object_set(c, child, "marker", &value) == FVM_OK);
            assert(fvm_set_global(c, "Infinity", &value) == FVM_OK); value = object(child); assert(fvm_set_global(c, "NaN", &value) == FVM_OK);
            assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
            if (version) check(c, c->global, version);
            else {
                assert(fvm_get_global(c, "NaN", &value) == FVM_OK && value.object == child);
                assert(fvm_object_get(c, child, "marker", &value) == FVM_OK && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
                assert(fvm_get_global(c, "Infinity", &value) == FVM_OK && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
            }
            FVM_Object *holder = 0; assert(fvm_object_create(c, &holder) == FVM_OK); assert(fvm_gc_pin(c, holder) == FVM_OK);
            holder->accessors["NaN"] = c->global->accessors.find("NaN")->second;
            holder->accessors["Infinity"] = c->global->accessors.find("Infinity")->second;
            FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); value = object(holder);
            assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &value, 0) == FVM_OK);
            FVM_Object *copy = value.object; assert(fvm_gc_pin(other, copy) == FVM_OK);
            assert(fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK);
            if (!version) {
                assert(fvm_object_get(other, copy, "NaN", &value) == FVM_OK && value.type == FVM_VALUE_OBJECT);
                assert(fvm_object_get(other, value.object, "marker", &value) == FVM_OK && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
                assert(fvm_object_get(other, copy, "Infinity", &value) == FVM_OK && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
            } else check(other, copy, 0);
            assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK); check(other, copy, version ? version : 4);
            assert(c->gc_frames == 0 && other->gc_frames == 0 && !c->avm1_execution_depth && !other->avm1_execution_depth);
            fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
