/* Watch callbacks use depth zero independently of function storage. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include "function_adapter_super.cpp"
#include "../src/core/call_scope_constants.h"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *keys[] = {"slot", "5", "n\0m"}; const size_t sizes[] = {4, 1, 3};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 5; ++kind)
    for (unsigned chain = 0; chain < 4; ++chain)
    for (unsigned location = 0; location < 5; ++location)
    for (unsigned key = 0; key < 3; ++key)
    for (unsigned route = 0; route < 4; ++route) {
        if ((kind && version < 7) || (route >= 2 && !fvm_jit_available())) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *nodes[4]; FVM_Value value, args[2], out;
        for (unsigned i = 0; i < 4; ++i) { assert(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK && fvm_gc_pin(c, nodes[i]) == FVM_OK); fvm_value_number(&value, i); assert(fvm_object_set(c, nodes[i], "marker", &value) == FVM_OK); }
        FVM_Object *function = adapter_super_function(c, kind);
        if (location) { value = class_value(function); assert(fvm_object_set(c, nodes[location - 1], "storedFunction", &value) == FVM_OK); }
        fvm_value_string_bytes(args, keys[key], sizes[key]); args[1] = class_value(function);
        assert(fvm_object_call(c, nodes[0], "watch", 2, args, &out) == FVM_OK && out.boolean);
        for (unsigned i = 0; i < 4; ++i) assert(fvm_object_set_prototype(c, nodes[i], i < chain ? nodes[i + 1] : 0) == FVM_OK);
        value = class_value(nodes[0]); assert(fvm_set_global(c, "watchSuperTarget", &value) == FVM_OK);
        assert(fvm_set_global(c, "watchSuperKey", args) == FVM_OK);
        if (!route) { fvm_value_number(&value, 81); assert(fvm_object_set_bytes(c, nodes[0], keys[key], sizes[key], &value) == FVM_OK); }
        else { Program code; code.variable("watchSuperTarget"); code.variable("watchSuperKey"); code.number(81); code.op(0x4f); code.number(1); code.finish(); boxing_run(c, code, route - 1, FVM_OK); }
        assert(fvm_object_get_bytes(c, nodes[0], keys[key], sizes[key], &out) == FVM_OK);
        if (kind == 2 || kind == 3) assert(out.type == FVM_VALUE_UNDEFINED);
        else {
            assert(out.type == FVM_VALUE_OBJECT && out.object->resource_kind == FVM_RESOURCE_SUPER);
            assert(out.object->resource_result.object->properties["depth"].number == 0);
            FVM_Object *facade = out.object; assert(fvm_gc_pin(c, facade) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_object_get(c, facade, "marker", &out) == FVM_OK);
            if (chain) assert(out.type == FVM_VALUE_NUMBER && out.number == 1); else assert(out.type == FVM_VALUE_UNDEFINED);
        }
        assert(!c->gc_frames && !c->gc_native_depth && c->watch_running.empty() && !c->calling_function && !c->calling_owner && c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
