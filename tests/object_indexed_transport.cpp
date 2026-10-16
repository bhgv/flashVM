/* Ordinary indexed method calls retain data, functions and nested super after graph transport. */
#define FVM_SUPER_WRITE_FIXTURE_ONLY
#include "super_property_write.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned array = 0; array < 2; ++array) {
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK);
        version_load(c, version); version_load(other, version); assert(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK);
        FVM_Object *receiver = 0, *parent = 0, *far = 0, *method = 0;
        assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
        assert(fvm_construct(c, array ? "Array" : "Object", 0, 0, &parent) == FVM_OK && fvm_construct(c, "Object", 0, 0, &far) == FVM_OK);
        FVM_Value value, out; fvm_value_string_bytes(&value, "r\0s", 3); assert(fvm_table_set_index(c, parent, 0, &value) == FVM_OK);
        value = class_value(version_function(c, 91)); assert(fvm_table_set_index(c, far, 5, &value) == FVM_OK);
        Program body; body.number(0); body.variable("super"); body.text("5"); body.op(0x52); body.finish();
        assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &method) == FVM_OK);
        value = class_value(method); assert(fvm_table_set_index(c, parent, 5, &value) == FVM_OK);
        assert(fvm_object_set_prototype(c, far, receiver->prototype) == FVM_OK && fvm_object_set_prototype(c, parent, far) == FVM_OK && fvm_object_set_prototype(c, receiver, parent) == FVM_OK);
        value = class_value(receiver); unsigned char *wire = 0; size_t size = 0;
        assert(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK && fvm_value_deserialize(other, wire, size, &out) == FVM_OK); fvm_serialized_free(wire);
        FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
        assert(fvm_object_get(other, copy, "0", &out) == FVM_OK && out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "r\0s", 3));
        assert(fvm_object_call(other, copy, "5", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 91);
        assert(!other->gc_frames && !other->gc_native_depth && !other->calling_owner && !other->calling_function);
        fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
