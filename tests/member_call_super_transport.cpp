/* Graph-restored own and inherited named methods keep their captured superclass depth. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include "function_adapter_super.cpp"
#include "../src/standard/super.hpp"
#include "../src/core/call_scope.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned parent_call = 0; parent_call < 3; ++parent_call) {
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK);
        version_load(c, version); version_load(other, version); assert(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK);
        FVM_Object *nodes[4], *resolver = 0; FVM_Value value, out;
        for (unsigned i = 0; i < 4; ++i) { assert(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK); fvm_value_number(&value, i); assert(fvm_object_set(c, nodes[i], "marker", &value) == FVM_OK); }
        FVM_Object *method = adapter_super_function(c, 0); value = class_value(method); assert(fvm_object_set_bytes(c, nodes[parent_call], "n\0m", 3, &value) == FVM_OK);
        for (unsigned i = 0; i < 4; ++i) assert(fvm_object_set_prototype(c, nodes[i], i < 3 ? nodes[i + 1] : 0) == FVM_OK);
        value = class_value(nodes[0]); const unsigned depth = parent_call ? parent_call : 1;
        unsigned char *wire = 0; size_t size = 0; assert(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK && fvm_value_deserialize(other, wire, size, &out) == FVM_OK); fvm_serialized_free(wire);
        FVM_Object *target = out.object; assert(fvm_gc_pin(other, target) == FVM_OK && fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
        assert(fvm_object_call_bytes(other, target, "n\0m", 3, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT && out.object->resource_result.object->properties["depth"].number == depth);
        assert(fvm_gc_pin(other, out.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK && fvm_object_get(other, out.object, "marker", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == depth + 1);
        assert(!other->resolve_depth && !other->gc_frames && !other->gc_native_depth); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
