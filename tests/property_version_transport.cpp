/* UInt16 attributes, counted names and cycles survive graph transport and version replacement. */
#include "property_version_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned source = 4; source <= 8; ++source)
    for (unsigned destination = 0; destination <= 8; ++destination) {
        if (destination && destination < 4) continue;
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK); version_load(c, source); if (destination) version_load(other, destination); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *object = 0; FVM_Value value, out, args[4]; assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK); value = class_value(object); assert(fvm_object_set_bytes(c, object, "K\0Q", 3, &value) == FVM_OK);
        args[0] = value; fvm_value_string_bytes(args + 1, "k\0q", 3); fvm_value_number(args + 2, 0x1000); fvm_value_number(args + 3, 0); assert(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK);
        unsigned char *data = 0; size_t bytes = 0; assert(fvm_value_serialize(c, &value, &data, &bytes) == FVM_OK && fvm_value_deserialize(other, data, bytes, &out) == FVM_OK); fvm_serialized_free(data);
        FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
        unsigned flags = 0; assert(fvm_object_property_flags_bytes(other, copy, "K\0Q", 3, &flags) == FVM_OK && flags == 0x1000);
        assert(fvm_object_get_bytes(other, copy, "K\0Q", 3, &out) == FVM_OK); if (destination && destination < 8) assert(out.type == FVM_VALUE_UNDEFINED); else assert(out.type == FVM_VALUE_OBJECT && out.object == copy);
        version_load(other, 8); assert(fvm_object_get_bytes(other, copy, "K\0Q", 3, &out) == FVM_OK && out.object == copy);
        version_load(other, 5); assert(fvm_object_get_bytes(other, copy, "K\0Q", 3, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        /* A writable hidden field becomes visible after replacement and drops version bits. */
        fvm_value_number(&out, 42); assert(fvm_object_set_bytes(other, copy, "K\0Q", 3, &out) == FVM_OK && fvm_object_property_flags_bytes(other, copy, "K\0Q", 3, &flags) == FVM_OK && !flags);
        assert(fvm_object_get_bytes(other, copy, "K\0Q", 3, &out) == FVM_OK && out.number == 42); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
