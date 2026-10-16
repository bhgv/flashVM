/* Counted names and deeper script setters retain lookup behavior after graph transport. */
#include "property_version_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *keys[] = {"slot", "5", "+5", "n\0m"}; const size_t sizes[] = {4, 1, 2, 3};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned key = 0; key < 4; ++key)
    for (unsigned array = 0; array < 2; ++array) {
        FVM_Context *c = 0, *other = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK);
        version_load(c, version); version_load(other, version); assert(fvm_standard_install(c) == FVM_OK && fvm_standard_install(other) == FVM_OK);
        FVM_Object *receiver = 0, *middle = 0, *far = 0, *set = 0;
        assert(fvm_construct(c, array ? "Array" : "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &middle) == FVM_OK && fvm_construct(c, "Object", 0, 0, &far) == FVM_OK);
        const unsigned char body[] = {0x96,6,0,0,'t','h','i','s',0,0x1c,0x96,13,0,0,'c','a','l','l','e','d',0,7,1,0,0,0,0x4f,0};
        assert(fvm_function_create_avm1(c, body, sizeof(body), 0, 0, 0, &set) == FVM_OK);
        FVM_Value args[3], value, out; fvm_value_number(&value, 9);
        assert(fvm_object_set_bytes(c, middle, keys[key], sizes[key], &value) == FVM_OK);
        fvm_value_string_bytes(args, keys[key], sizes[key]); args[1] = class_value(version_function(c, 42)); args[2] = class_value(set);
        assert(fvm_object_call(c, far, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        assert(fvm_object_set_prototype(c, far, receiver->prototype) == FVM_OK && fvm_object_set_prototype(c, middle, far) == FVM_OK && fvm_object_set_prototype(c, receiver, middle) == FVM_OK);
        value = class_value(receiver); unsigned char *wire = 0; size_t bytes = 0;
        assert(fvm_value_serialize(c, &value, &wire, &bytes) == FVM_OK && fvm_value_deserialize(other, wire, bytes, &out) == FVM_OK); fvm_serialized_free(wire);
        FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
        fvm_value_number(&value, 81); assert(fvm_object_set_bytes(other, copy, keys[key], sizes[key], &value) == FVM_OK);
        assert(fvm_object_get(other, copy, "called", &out) == FVM_OK && out.number == 1);
        int own = 1; assert(fvm_object_has_own_bytes(other, copy, keys[key], sizes[key], &own) == FVM_OK && !own);
        assert(fvm_object_get_bytes(other, copy, keys[key], sizes[key], &out) == FVM_OK && out.number == 9);
        if (array) assert(fvm_object_get(other, copy, "length", &out) == FVM_OK && out.number == 0);
        assert(!other->gc_frames && !other->gc_native_depth && !other->calling_setter_owner);
        fvm_context_destroy(other); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
