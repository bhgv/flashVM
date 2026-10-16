/* Loaded resolved and qualified CallFunction use depth zero independent of property ownership. */
#define FVM_PROPERTY_CALLBACK_SUPER_FIXTURE_ONLY
#include "property_callback_super.cpp"
#include "../src/core/call_scope.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 5; ++kind)
    for (unsigned chain = 0; chain < 4; ++chain)
    for (unsigned storage = 0; storage <= chain; ++storage)
    for (unsigned virtual_slot = 0; virtual_slot < 2; ++virtual_slot)
    for (unsigned route = 0; route < 5; ++route) {
        if ((kind && version < 7) || (route >= 3 && !fvm_jit_available())) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *nodes[4]; FVM_Value value, out, name;
        for (unsigned i = 0; i < 4; ++i) { assert(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK && fvm_gc_pin(c, nodes[i]) == FVM_OK); fvm_value_number(&value, i); assert(fvm_object_set(c, nodes[i], "marker", &value) == FVM_OK); }
        FVM_Object *function = adapter_super_function(c, kind); value = class_value(function);
        if (!virtual_slot) assert(fvm_object_set(c, nodes[storage], "slot", &value) == FVM_OK);
        else {
            assert(fvm_object_set(c, nodes[0], "returned", &value) == FVM_OK); Program body; body.variable("this"); body.text("returned"); body.op(0x4e); body.finish(); FVM_Object *getter = 0;
            assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &getter) == FVM_OK); callback_super_property(c, nodes[storage], "slot", 4, getter, version_function(c, 0));
        }
        for (unsigned i = 0; i < 4; ++i) assert(fvm_object_set_prototype(c, nodes[i], i < chain ? nodes[i + 1] : 0) == FVM_OK);
        value = class_value(nodes[0]); assert(fvm_set_global(c, "qualified", &value) == FVM_OK);
        if (!route) { value = class_value(function); fvm_value_string_bytes(&name, "n\0m", 3); assert(fvm_exec_resolved_call(c, 0, &name, &value, 1, nodes[0], 0, 0, &out) == FVM_OK); }
        else if (route == 1) { fvm_value_string(&name, "qualified:slot"); assert(fvm_exec_call_function(c, 0, 0, 0, 0, 0, &name, 0, 0, &out) == FVM_OK); }
        else { Program code; code.number(0); code.text("qualified:slot"); code.op(0x3d); code.finish(); out = boxing_run(c, code, route - 2, FVM_OK); }
        if (kind == 2 || kind == 3) assert(out.type == FVM_VALUE_UNDEFINED);
        else {
            assert(out.type == FVM_VALUE_OBJECT && out.object->resource_kind == FVM_RESOURCE_SUPER && out.object->resource_result.object->properties["depth"].number == 0);
            FVM_Object *facade = out.object; assert(fvm_gc_pin(c, facade) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, facade, "marker", &out) == FVM_OK);
            if (chain) assert(out.type == FVM_VALUE_NUMBER && out.number == 1); else assert(out.type == FVM_VALUE_UNDEFINED);
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
