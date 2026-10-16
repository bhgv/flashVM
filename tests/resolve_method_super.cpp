/* Resolver-returned methods have explicit depth one, relative to an explicit super facade. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include "function_adapter_super.cpp"
#include "../src/core/call_scope.hpp"
#include "../src/standard/super.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 5; ++kind)
    for (unsigned level = 0; level < 5; ++level)
    for (unsigned route = 0; route < 4; ++route) {
        if ((kind && version < 7) || (route >= 2 && !fvm_jit_available())) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *nodes[6]; FVM_Value value, out;
        for (unsigned i = 0; i < 6; ++i) { assert(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK && fvm_gc_pin(c, nodes[i]) == FVM_OK); fvm_value_number(&value, i); assert(fvm_object_set(c, nodes[i], "marker", &value) == FVM_OK); }
        FVM_Object *method = adapter_super_function(c, kind); value = class_value(method); assert(fvm_object_set(c, nodes[0], "returned", &value) == FVM_OK);
        Program body; body.variable("this"); body.text("returned"); body.op(0x4e); body.finish(); FVM_Object *resolver = 0; assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &resolver) == FVM_OK);
        value = class_value(resolver); assert(fvm_object_set(c, nodes[5], "__resolve", &value) == FVM_OK);
        for (unsigned i = 0; i < 6; ++i) assert(fvm_object_set_prototype(c, nodes[i], i < 5 ? nodes[i + 1] : 0) == FVM_OK);
        FVM_Object *target = nodes[0]; unsigned expected_depth = 1;
        if (level) { FVM_CallScope scope(c, 0, method, 0, level - 1); assert(fvm_super_create(c, method, nodes[0], &out) == FVM_OK && out.type == FVM_VALUE_OBJECT); target = out.object; expected_depth = level; }
        value = class_value(target); assert(fvm_set_global(c, "resolvedSuperTarget", &value) == FVM_OK);
        if (!route) assert(fvm_object_call_bytes(c, target, "n\0m", 3, 0, 0, &out) == FVM_OK);
        else { fvm_value_string_bytes(&value, "n\0m", 3); assert(fvm_set_global(c, "resolvedSuperKey", &value) == FVM_OK); Program code; code.number(0); code.variable("resolvedSuperTarget"); code.variable("resolvedSuperKey"); code.op(0x52); code.finish(); out = boxing_run(c, code, route - 1, FVM_OK); }
        if (kind == 2 || kind == 3) assert(out.type == FVM_VALUE_UNDEFINED);
        else {
            assert(out.type == FVM_VALUE_OBJECT && out.object->resource_kind == FVM_RESOURCE_SUPER && out.object->resource_result.object->properties["depth"].number == expected_depth);
            FVM_Object *facade = out.object; assert(fvm_gc_pin(c, facade) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
            int removed = 0; assert(fvm_object_delete(c, nodes[5], "__resolve", &removed) == FVM_OK && removed);
            assert(fvm_object_get(c, facade, "marker", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == expected_depth + 1);
        }
        assert(!c->resolve_depth && !c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
