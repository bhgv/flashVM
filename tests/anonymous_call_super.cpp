/* Loaded anonymous CallMethod invokes Functions at depth zero independently of storage. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include "function_adapter_super.cpp"
#include "../src/core/call_scope.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 5; ++kind)
    for (unsigned chain = 0; chain < 4; ++chain)
    for (unsigned storage = 0; storage < 5; ++storage)
    for (unsigned undefined_name = 0; undefined_name < 2; ++undefined_name)
    for (unsigned route = 0; route < 4; ++route) {
        if ((kind && version < 7) || (route >= 2 && !fvm_jit_available())) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *nodes[4]; nodes[0] = c->global; FVM_Value value, out;
        for (unsigned i = 1; i < 4; ++i) { assert(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK && fvm_gc_pin(c, nodes[i]) == FVM_OK); fvm_value_number(&value, i); assert(fvm_object_set(c, nodes[i], "marker", &value) == FVM_OK); }
        FVM_Object *function = adapter_super_function(c, kind); value = class_value(function); assert(fvm_set_global(c, "anonymousFunction", &value) == FVM_OK);
        if (kind) { int removed = 0; assert(fvm_object_delete(c, c->global, "adapterFunction2", &removed) == FVM_OK && removed); }
        if (storage) assert(fvm_object_set(c, nodes[storage - 1], "storedFunction", &value) == FVM_OK);
        for (unsigned i = 0; i < 4; ++i) assert(fvm_object_set_prototype(c, nodes[i], i < chain ? nodes[i + 1] : 0) == FVM_OK);
        if (!route) { int removed = 0; assert(fvm_object_delete(c, c->global, "anonymousFunction", &removed) == FVM_OK && removed); FVM_Value name; if (undefined_name) fvm_value_undefined(&name); else fvm_value_string(&name, ""); assert(fvm_exec_call_method(c, 0, &value, &name, c->global, 0, 0, &out) == FVM_OK); }
        else { Program code; code.number(0); code.variable("anonymousFunction"); code.variable("this"); code.text("anonymousFunction"); code.op(0x3a); code.op(0x17); if (undefined_name) code.variable("undefined"); else code.text(""); code.op(0x52); code.finish(); out = boxing_run(c, code, route - 1, FVM_OK); }
        assert(out.type == FVM_VALUE_UNDEFINED);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(!c->calling_this_undefined && !c->script_this_undefined && !c->script_this && !c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
