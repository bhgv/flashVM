#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
static FVM_Result unexpected(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ assert(false); return FVM_ERROR; }
static void text_result(const FVM_Value &out)
{ assert(out.type == FVM_VALUE_STRING && out.string && std::strstr(out.string, "GMT") && fvm_value_string_size(&out) > 20); }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        if (version) assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value klass, arg, value, out; FVM_Object *bad = 0, *fn = 0, *date = 0;
        assert(fvm_get_global(c, "Date", &klass) == FVM_OK); fvm_value_number(&arg, 81);
        assert(fvm_call(c, "Date", 1, &arg, &out) == FVM_OK); if (version) text_result(out); else assert(out.type == FVM_VALUE_OBJECT);
        assert(fvm_construct(c, "Date", 1, &arg, &date) == FVM_OK); assert(fvm_object_call(c, date, "getTime", 0, 0, &out) == FVM_OK && out.number == 81);
        if (version) {
            assert(fvm_construct(c, "Object", 0, 0, &bad) == FVM_OK); assert(fvm_function_create_native(c, unexpected, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, bad, "valueOf", &value) == FVM_OK); assert(fvm_object_set(c, bad, "toString", &value) == FVM_OK); arg = object(bad); assert(fvm_gc_pin(c, bad) == FVM_OK);
            assert(fvm_call(c, "Date", 1, &arg, &out) == FVM_OK); text_result(out); assert(fvm_set_global(c, "saved", &out) == FVM_OK);
            assert(fvm_function_invoke(c, klass.object, 0, 1, &arg, &out) == FVM_OK); text_result(out);
            const unsigned char code[] = {0x96,3,0,0,'r',0,0x96,5,0,7,0,0,0,0,0x96,6,0,0,'D','a','t','e',0,0x3d,0x1d,0}; assert(fvm_avm1_execute(c, code, sizeof(code)) == FVM_OK); assert(fvm_get_global(c, "r", &out) == FVM_OK); text_result(out);
            const unsigned char construct[] = {0x96,3,0,0,'r',0,0x96,5,0,7,0,0,0,0,0x96,6,0,0,'D','a','t','e',0,0x40,0x1d,0}; assert(fvm_avm1_execute(c, construct, sizeof(construct)) == FVM_OK); assert(fvm_get_global(c, "r", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT && out.object->resource_kind == 34);
            assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK); assert(fvm_get_global(c, "saved", &out) == FVM_OK); text_result(out);
            assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK);
        }
        FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); assert(fvm_channel_send_value(c, channel, &klass, 0) == FVM_OK); assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK); fvm_value_number(&arg, 81); assert(fvm_function_invoke(other, copy, 0, 1, &arg, &out) == FVM_OK); if (version) text_result(out); else assert(out.type == FVM_VALUE_OBJECT); assert(fvm_function_construct(other, copy, 1, &arg, &date) == FVM_OK); assert(fvm_object_call(other, date, "getTime", 0, 0, &out) == FVM_OK && out.number == 81);
        assert(c->gc_frames == 0 && other->gc_frames == 0 && !c->primitive_depth && !other->primitive_depth); fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
