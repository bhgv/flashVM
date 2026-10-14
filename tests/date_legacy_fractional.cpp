#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include "flashvm/serialization.h"
#include <cmath>
#include <cstdio>
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; if (version) { assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK); } assert(fvm_standard_install(c) == FVM_OK);
        const double inputs[] = {2.75,-2.75,-0.25,0.25,2000000000.875,-2000000000.875};
        FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK);
        FVM_Value arg, out, value; FVM_Object *date = 0;
        for (unsigned i = 0; i < 6; ++i) {
            const double host = inputs[i] < 0 ? -std::floor(-inputs[i]) : std::floor(inputs[i]); fvm_value_number(&arg, inputs[i]); assert(fvm_construct(c, "Date", 1, &arg, &date) == FVM_OK); assert(fvm_object_call(c, date, "getTime", 0, 0, &out) == FVM_OK && out.number == (version ? inputs[i] : host));
            value = object(date); assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); assert(fvm_object_call(other, copy, "getTime", 0, 0, &out) == FVM_OK && out.number == (version ? inputs[i] : host));
            assert(fvm_object_call(c, date, "setTime", 1, &arg, &out) == FVM_OK && out.number == (version ? std::floor(inputs[i]) : host)); assert(fvm_object_call(c, date, "getTime", 0, 0, &out) == FVM_OK && out.number == (version ? std::floor(inputs[i]) : host));
        }
        fvm_value_number(&arg, 259200000); assert(fvm_construct(c, "Date", 1, &arg, &date) == FVM_OK); assert(fvm_object_call(c, date, "getDate", 0, 0, &out) == FVM_OK); int day = (int)out.number; char expected[8], actual[16]; std::sprintf(expected, version ? "%d" : "%02d", day); assert(fvm_object_call(c, date, "toString", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_STRING); assert(std::sscanf(out.string, "%*s %*s %15s", actual) == 1 && !std::strcmp(actual, expected));
        const double invalid[] = {HUGE_VAL,-HUGE_VAL,HUGE_VAL-HUGE_VAL,8640000000000001.0}; for (unsigned i = 0; i < 4; ++i) { fvm_value_number(&arg, invalid[i]); assert(fvm_object_call(c, date, "setTime", 1, &arg, &out) == FVM_OK && out.number != out.number); }
        for (unsigned i = 0; i < 3; ++i) {
            date->resource_result.number = i ? 8640000000000001.0 : HUGE_VAL; if (i == 2) date->resource_result.type = FVM_VALUE_BOOLEAN;
            value = object(date); unsigned char *data = 0; size_t size = 0; unsigned long before = 0, after = 0; FVM_Result encoded = fvm_value_serialize(c, &value, &data, &size); if (i == 2) { assert(encoded == FVM_UNSUPPORTED && !data && !size); continue; } assert(encoded == FVM_OK); assert(fvm_gc_object_count(other, &before) == FVM_OK); assert(fvm_value_deserialize(other, data, size, &out) == FVM_INVALID_ARGUMENT && out.type == FVM_VALUE_UNDEFINED); assert(fvm_gc_object_count(other, &after) == FVM_OK && before == after); fvm_serialized_free(data);
        }
        assert(c->gc_frames == 0 && other->gc_frames == 0 && !c->primitive_depth && !other->primitive_depth); fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
