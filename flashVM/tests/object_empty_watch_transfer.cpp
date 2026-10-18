/* Empty-name watchers survive graph transfer and remain removable after traced GC. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static void verify(FVM_Context *c, FVM_Object *o)
{
    fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator watch = o->watchers.find("");
    assert(watch != o->watchers.end());
    FVM_Value value, out, args[3];
    assert(fvm_object_get(c, watch->second, "userData", &value) == FVM_OK && value.object == o);
    assert(fvm_object_get(c, o, "", &value) == FVM_OK && value.number == 9);
    fvm_value_number(&value, 81); assert(fvm_object_set(c, o, "", &value) == FVM_OK);
    assert(fvm_object_get(c, o, "", &value) == FVM_OK && value.number == 9);
    assert(fvm_object_get(c, o, "called", &value) == FVM_OK && value.type == FVM_VALUE_UNDEFINED);
    fvm_value_string(args, ""); assert(fvm_object_call(c, o, "unwatch", 1, args, &out) == FVM_OK && out.boolean);
    assert(fvm_object_call(c, o, "unwatch", 1, args, &out) == FVM_OK && !out.boolean);
    assert(c->gc_frames == 0 && c->watch_running.empty());
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK); assert(fvm_context_create(rt, &other) == FVM_OK);
        assert(fvm_standard_install(c) == FVM_OK); assert(fvm_standard_install(other) == FVM_OK);
        FVM_Object *o = 0, *fn = 0; FVM_Value value, args[3], out;
        assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set(c, o, "", &value) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK);
        const unsigned char body[] = {0x96,6,0,0,'t','h','i','s',0,0x1c,0x96,13,0,0,'c','a','l','l','e','d',0,7,1,0,0,0,0x4f,0};
        assert(fvm_function_create_avm1(c, body, sizeof(body), 0, 0, 0, &fn) == FVM_OK);
        fvm_value_string(args, ""); args[1] = object(fn); args[2] = object(o);
        assert(fvm_object_call(c, o, "watch", 3, args, &out) == FVM_OK && out.boolean);
        assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
        FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK); value = object(o);
        assert(fvm_channel_send_value(c, channel, &value, 0) == FVM_OK);
        assert(fvm_channel_receive_value(other, channel, &out, 0) == FVM_OK); assert(fvm_gc_pin(other, out.object) == FVM_OK);
        assert(fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(other, 0) == FVM_OK);
        verify(c, o); verify(other, out.object);
        fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
