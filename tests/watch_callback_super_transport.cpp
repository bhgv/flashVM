/* Serialized pure AVM1 watchers preserve depth zero and counted property keys. */
#define FVM_FUNCTION_ADAPTER_SUPER_FIXTURE_ONLY
#include "function_adapter_super.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned chain = 0; chain < 3; ++chain) {
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK);
        version_load(c, version); version_load(other, version); assert(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK);
        FVM_Object *nodes[3]; FVM_Value args[2], value, out;
        for (unsigned i = 0; i < 3; ++i) { assert(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK); fvm_value_number(&value, i); assert(fvm_object_set(c, nodes[i], "marker", &value) == FVM_OK); }
        fvm_value_string_bytes(args, "n\0m", 3); args[1] = class_value(adapter_super_function(c, 0)); assert(fvm_object_call(c, nodes[0], "watch", 2, args, &out) == FVM_OK && out.boolean);
        for (unsigned i = 0; i < 3; ++i) assert(fvm_object_set_prototype(c, nodes[i], i < chain ? nodes[i + 1] : 0) == FVM_OK);
        value = class_value(nodes[0]); unsigned char *wire = 0; size_t size = 0;
        assert(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK && fvm_value_deserialize(other, wire, size, &out) == FVM_OK); fvm_serialized_free(wire);
        FVM_Object *receiver = out.object; assert(fvm_gc_pin(other, receiver) == FVM_OK && fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
        fvm_value_number(&value, 81); assert(fvm_object_set_bytes(other, receiver, "n\0m", 3, &value) == FVM_OK);
        assert(fvm_object_get_bytes(other, receiver, "n\0m", 3, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT && out.object->resource_result.object->properties["depth"].number == 0);
        assert(fvm_object_get(other, out.object, "marker", &out) == FVM_OK); if (chain) assert(out.type == FVM_VALUE_NUMBER && out.number == 1); else assert(out.type == FVM_VALUE_UNDEFINED);
        assert(fvm_gc_collect(other, 0) == FVM_OK && other->watch_running.empty() && !other->gc_frames && !other->gc_native_depth);
        fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
