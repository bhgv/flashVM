/* Restored virtual descriptors retain depth-one getter/setter super and counted keys/data. */
#define FVM_PROPERTY_CALLBACK_SUPER_FIXTURE_ONLY
#include "property_callback_super.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char key[] = "n\0m";
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned location = 0; location < 3; ++location) {
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK);
        version_load(c, version); version_load(other, version); assert(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK);
        FVM_Object *nodes[3], *write = 0; FVM_Value value, out;
        for (unsigned i = 0; i < 3; ++i) { assert(fvm_construct(c, "Object", 0, 0, nodes + i) == FVM_OK && fvm_gc_pin(c, nodes[i]) == FVM_OK); fvm_value_number(&value, i); assert(fvm_object_set(c, nodes[i], "marker", &value) == FVM_OK); }
        Program body; body.variable("this"); body.text("savedSuper"); body.variable("super"); body.op(0x4f); body.number(1); body.finish();
        assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &write) == FVM_OK);
        callback_super_property(c, nodes[location], key, sizeof(key) - 1, adapter_super_function(c, 0), write);
        assert(fvm_object_set_prototype(c, nodes[0], nodes[1]) == FVM_OK && fvm_object_set_prototype(c, nodes[1], nodes[2]) == FVM_OK);
        value = class_value(nodes[0]); unsigned char *wire = 0; size_t size = 0;
        assert(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK && fvm_value_deserialize(other, wire, size, &out) == FVM_OK); fvm_serialized_free(wire);
        FVM_Object *receiver = out.object; assert(fvm_gc_pin(other, receiver) == FVM_OK && fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
        assert(fvm_object_get_bytes(other, receiver, key, sizeof(key) - 1, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT && out.object->resource_kind == FVM_RESOURCE_SUPER);
        assert(out.object->resource_result.object->properties["depth"].number == 1 && fvm_object_get(other, out.object, "marker", &out) == FVM_OK && out.number == 2);
        fvm_value_string_bytes(&value, "r\0s", 3); assert(fvm_object_set_bytes(other, receiver, key, sizeof(key) - 1, &value) == FVM_OK);
        assert(fvm_object_get(other, receiver, "savedSuper", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT && out.object->resource_result.object->properties["depth"].number == 1);
        assert(fvm_gc_collect(other, 0) == FVM_OK && !other->gc_frames && !other->gc_native_depth && !other->calling_setter_owner);
        fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
