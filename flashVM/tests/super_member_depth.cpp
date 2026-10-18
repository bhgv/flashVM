/* Explicit-super methods capture facade-relative lookup depth before getters execute. */
#define FVM_PROPERTY_CALLBACK_SUPER_FIXTURE_ONLY
#include "property_callback_super.cpp"
#include "../src/core/call_scope.hpp"
#include "../src/standard/super.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *keys[] = {"slot", "5", "n\0m"}; const size_t sizes[] = {4,1,3};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 5; ++kind)
    for (unsigned base = 0; base < 3; ++base)
    for (unsigned location = 0; location < 3; ++location)
    for (unsigned virtual_slot = 0; virtual_slot < 2; ++virtual_slot)
    for (unsigned key = 0; key < 3; ++key)
    for (unsigned route = 0; route < 4; ++route) {
        if ((kind && version < 7) || (route >= 2 && !fvm_jit_available())) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *nodes[7]; FVM_Value value, out;
        for (unsigned i = 0; i < 7; ++i) { assert(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK && fvm_gc_pin(c, nodes[i]) == FVM_OK); fvm_value_number(&value, i); assert(fvm_object_set(c, nodes[i], "marker", &value) == FVM_OK); }
        FVM_Object *method = adapter_super_function(c, kind); value = class_value(method);
        if (!virtual_slot) assert(fvm_object_set_bytes(c, nodes[base + location + 1], keys[key], sizes[key], &value) == FVM_OK);
        else {
            assert(fvm_object_set(c, nodes[0], "returned", &value) == FVM_OK);
            Program body; body.variable("this"); body.text("returned"); body.op(0x4e); body.finish(); FVM_Object *getter = 0;
            assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &getter) == FVM_OK);
            callback_super_property(c, nodes[base + location + 1], keys[key], sizes[key], getter, version_function(c, 0));
        }
        for (unsigned i = 0; i < 7; ++i) assert(fvm_object_set_prototype(c, nodes[i], i < 6 ? nodes[i + 1] : 0) == FVM_OK);
        { FVM_CallScope scope(c, 0, method, 0, base); assert(fvm_super_create(c, method, nodes[0], &value) == FVM_OK); } FVM_Object *target = value.object; assert(fvm_set_global(c, "memberDepthTarget", &value) == FVM_OK); fvm_value_string_bytes(&value, keys[key], sizes[key]); assert(fvm_set_global(c, "memberDepthKey", &value) == FVM_OK);
        if (!route) assert(fvm_object_call_bytes(c, target, keys[key], sizes[key], 0, 0, &out) == FVM_OK);
        else { Program code; code.number(0); code.variable("memberDepthTarget"); code.variable("memberDepthKey"); code.op(0x52); code.finish(); out = boxing_run(c, code, route - 1, FVM_OK); }
        if (kind == 2 || kind == 3) assert(out.type == FVM_VALUE_UNDEFINED);
        else {
            const unsigned depth = base + location + 1;
            assert(out.type == FVM_VALUE_OBJECT && out.object->resource_kind == FVM_RESOURCE_SUPER && out.object->resource_result.object->properties["depth"].number == depth);
            FVM_Object *facade = out.object; assert(fvm_gc_pin(c, facade) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_object_get(c, facade, "marker", &out) == FVM_OK);
            assert(out.type == FVM_VALUE_NUMBER && out.number == depth + 1);
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
