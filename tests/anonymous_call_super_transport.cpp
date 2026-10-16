/* Pure AVM1 Function values restored into another context use that anonymous receiver at depth zero. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include "function_adapter_super.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK); version_load(c, version); version_load(other, version); assert(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK);
        FVM_Value value = class_value(adapter_super_function(c, 0)), out, name; unsigned char *wire = 0; size_t size = 0;
        assert(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK && fvm_value_deserialize(other, wire, size, &value) == FVM_OK); fvm_serialized_free(wire);
        assert(fvm_set_global(other, "anonymousFunction", &value) == FVM_OK); FVM_Object *parent = 0; assert(fvm_construct(other, "Object", 0, 0, &parent) == FVM_OK); fvm_value_number(&out, 81); assert(fvm_object_set(other, parent, "marker", &out) == FVM_OK && fvm_object_set_prototype(other, other->global, parent) == FVM_OK);
        if (!route) { int removed = 0; assert(fvm_object_delete(other, other->global, "anonymousFunction", &removed) == FVM_OK && removed); fvm_value_undefined(&name); assert(fvm_exec_call_method(other, 0, &value, &name, other->global, 0, 0, &out) == FVM_OK); }
        else { Program code; code.number(0); code.variable("anonymousFunction"); code.variable("this"); code.text("anonymousFunction"); code.op(0x3a); code.op(0x17); code.text(""); code.op(0x52); code.finish(); out = boxing_run(other, code, route - 1, FVM_OK); }
        assert(out.type == FVM_VALUE_OBJECT && out.object->resource_result.object->properties["depth"].number == 0 && out.object->resource_result.object->properties["receiver"].object == other->global);
        assert(fvm_gc_pin(other, out.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK && fvm_object_get(other, out.object, "marker", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 81);
        assert(!other->gc_frames && !other->gc_native_depth && !other->calling_function && !other->calling_owner); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
