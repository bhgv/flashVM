/* A super descriptor is a non-Function getter/setter in loaded AS; direct super remains callable. */
#define main super_accessor_fixture_main
#include "avm1_super.cpp"
#undef main
#include "../src/core/runtime.hpp"
#include "../src/standard/base_state.hpp"
static void super_accessor_setup(FVM_Context *c, FVM_Object **instance, FVM_Object **target, FVM_Object **facade)
{
    Bytes base_body; variable(base_body, "this"); text(base_body, "marker"); integer(base_body, 123); base_body.push_back(0x4f); base_body.push_back(0);
    Bytes empty; empty.push_back(0);
    FVM_Value base = define(c, "Base", 0, 0, base_body), sub = define(c, "Sub", 0, 0, empty); (void)base; extend(c, "Sub", "Base");
    Bytes escape; variable(escape, "super"); result(escape); FVM_Value method = define(c, "escape", 0, 0, escape);
    assert(fvm_object_set(c, prototype(c, sub), "escape", &method) == FVM_OK);
    FVM_Value value, reference;
    assert(fvm_function_construct(c, sub.object, 0, 0, instance) == FVM_OK && fvm_gc_pin(c, *instance) == FVM_OK);
    fvm_value_number(&value, 9); assert(fvm_object_set(c, *instance, "marker", &value) == FVM_OK);
    assert(fvm_object_call(c, *instance, "escape", 0, 0, &reference) == FVM_OK && reference.type == FVM_VALUE_OBJECT); *facade = reference.object;
    assert(fvm_construct(c, "Object", 0, 0, target) == FVM_OK && fvm_gc_pin(c, *target) == FVM_OK);
    assert(fvm_object_set(c, *target, "slot", &value) == FVM_OK);
    FVM_Value args[3], out; fvm_value_string(args, "slot"); args[1] = args[2] = reference;
    assert(fvm_object_call(c, *target, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
}
#ifndef FVM_SUPER_ACCESSOR_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 5) continue;
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK);
        FVM_Object *instance = 0, *target = 0, *facade = 0; FVM_Value out, marker, value;
        super_accessor_setup(c, &instance, &target, &facade);
        assert(fvm_object_get(c, target, "slot", &out) == FVM_OK);
        if (version) assert(out.type == FVM_VALUE_NUMBER && out.number == 9); else assert(out.type == FVM_VALUE_UNDEFINED);
        assert(fvm_object_get(c, instance, "marker", &marker) == FVM_OK && marker.number == (version ? 9 : 123));
        fvm_value_number(&value, 9); assert(fvm_object_set(c, instance, "marker", &value) == FVM_OK);
        fvm_value_number(&value, 81); assert(fvm_object_set(c, target, "slot", &value) == FVM_OK);
        assert(fvm_base_accessor_data(target->accessors.find(fvm_detail::string("slot"))->second, out) && out.number == 81);
        assert(fvm_object_get(c, instance, "marker", &marker) == FVM_OK && marker.number == (version ? 9 : 123));
        assert(fvm_function_invoke(c, facade, instance, 0, 0, &out) == FVM_OK);
        assert(fvm_object_get(c, instance, "marker", &marker) == FVM_OK && marker.number == 123);
        fvm_value_number(&value, 9); assert(fvm_object_set(c, instance, "marker", &value) == FVM_OK);
        fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = target;
        FVM_Channel *channel = 0; assert(fvm_channel_create(&channel) == FVM_OK && fvm_channel_send_value(c, channel, &value, 0) == FVM_OK && fvm_channel_receive_value(other, channel, &value, 0) == FVM_OK);
        assert(fvm_gc_pin(other, value.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
        FVM_Object *copy_super = value.object->accessors.find(fvm_detail::string("slot"))->second.root->properties["get"].object;
        FVM_Object *copy_receiver = copy_super->resource_result.object->properties["receiver"].object;
        assert(fvm_object_get(other, value.object, "slot", &out) == FVM_OK);
        if (version) assert(out.type == FVM_VALUE_NUMBER && out.number == 81); else assert(out.type == FVM_VALUE_UNDEFINED);
        assert(fvm_object_get(other, copy_receiver, "marker", &marker) == FVM_OK && marker.number == (version ? 9 : 123));
        assert(!c->gc_frames && !other->gc_frames && !c->gc_native_depth && !other->gc_native_depth);
        fvm_channel_destroy(channel); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
