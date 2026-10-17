/* Negative constructor length is visible, transportable and recoverable by indexed/length writes. */
#include "property_version_fixture.hpp"
#include <limits>
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned readonly = 0; readonly < 2; ++readonly) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0; FVM_Value input, out, length; fvm_value_number(&input, -3);
        assert(fvm_construct(c, "Array", 1, &input, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_object_call(c, array, "join", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_STRING && !fvm_value_string_size(&out));
        assert(fvm_object_call(c, array, "reverse", 0, 0, &out) == FVM_OK && out.object == array);
        assert(fvm_object_call(c, array, "sort", 0, 0, &out) == FVM_OK && out.object == array);
        assert(fvm_object_call(c, array, "concat", 0, 0, &out) == FVM_OK);
        assert(fvm_object_get(c, out.object, "length", &length) == FVM_OK && length.number == 0);
        assert(fvm_object_get(c, array, "length", &length) == FVM_OK && length.number == -3);
        if (readonly) assert(fvm_object_set_property_flags(c, array, "length", 4, 0) == FVM_OK);
        fvm_value_number(&input, 81);
        assert(fvm_table_set_index(c, array, 1, &input) == FVM_OK);
        assert(fvm_object_get(c, array, "length", &length) == FVM_OK && length.number == (readonly ? -3 : 2));
        assert(fvm_table_get_index(c, array, 1, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 81);
        FVM_Value value = class_value(array);
        assert(fvm_object_set(c, array, "cycle", &value) == FVM_OK);
        unsigned char *data = 0; size_t bytes = 0;
        assert(fvm_value_serialize(c, &value, &data, &bytes) == FVM_OK);
        FVM_Context *other = 0; assert(fvm_context_create(rt, &other) == FVM_OK);
        assert(fvm_value_deserialize(other, data, bytes, &out) == FVM_OK); fvm_serialized_free(data);
        assert(fvm_gc_pin(other, out.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
        assert(fvm_object_get(other, out.object, "length", &length) == FVM_OK && length.number == (readonly ? -3 : 2));
        assert(fvm_object_set_property_flags(other, out.object, "length", 0, 4) == FVM_OK);
        fvm_value_number(&input, 4); assert(fvm_object_set(other, out.object, "length", &input) == FVM_OK);
        assert(fvm_object_get(other, out.object, "length", &length) == FVM_OK && length.number == 4);
        fvm_context_destroy(other);
        assert(fvm_object_set_property_flags(c, array, "length", 0, 4) == FVM_OK);
        fvm_value_number(&input, 0); assert(fvm_object_set(c, array, "length", &input) == FVM_OK);
        assert(fvm_object_get(c, array, "length", &length) == FVM_OK && length.number == 0);
        /* Every valid typed length is now supported; absent native storage is malformed. */
        array->resource_result_valid = false;
        data = 0; bytes = 0;
        assert(fvm_value_serialize(c, &value, &data, &bytes) == FVM_UNSUPPORTED && !data && !bytes);
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
