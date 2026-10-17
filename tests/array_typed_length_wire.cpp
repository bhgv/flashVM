/* Typed Array length permits graph cycles and rejects malformed references before allocation. */
#include "property_version_fixture.hpp"
#include "zlib.h"
static void typed_wire_u32(unsigned char *out, unsigned long value)
{ for (unsigned i = 0; i < 4; ++i) out[i] = (unsigned char)(value >> (24 - 8 * i)); }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
    FVM_Object *array = 0; assert(fvm_object_create(c, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
    array->resource_kind = FVM_RESOURCE_ARRAY; array->resource_result_valid = true; array->resource_result = class_value(array);
    FVM_Value root = class_value(array), out; unsigned char *wire = 0; size_t bytes = 0;
    assert(fvm_value_serialize(c, &root, &wire, &bytes) == FVM_OK && bytes > 42 && wire[28] == 1 && wire[33] == FVM_VALUE_OBJECT);
    FVM_Context *other = 0; assert(fvm_context_create(rt, &other) == FVM_OK && fvm_value_deserialize(other, wire, bytes, &out) == FVM_OK);
    assert(out.object->resource_result.type == FVM_VALUE_OBJECT && out.object->resource_result.object == out.object);
    assert(fvm_gc_pin(other, out.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK); fvm_context_destroy(other);
    for (unsigned fault = 0; fault < 3; ++fault) {
        if (!fault) typed_wire_u32(wire + 34, 1);
        else { typed_wire_u32(wire + 34, 0); if (fault == 1) typed_wire_u32(wire + 38, 0); else { typed_wire_u32(wire + 38, 0xffffffffUL); wire[33] = 255; } }
        typed_wire_u32(wire + 12, (unsigned long)crc32(0, wire + 16, (uInt)(bytes - 16)));
        assert(fvm_context_create(rt, &other) == FVM_OK); unsigned long before = 0, after = 0; assert(fvm_gc_object_count(other, &before) == FVM_OK);
        assert(fvm_value_deserialize(other, wire, bytes, &out) == FVM_INVALID_ARGUMENT && out.type == FVM_VALUE_UNDEFINED);
        assert(fvm_gc_object_count(other, &after) == FVM_OK && before == after); fvm_context_destroy(other);
    }
    fvm_serialized_free(wire); fvm_context_destroy(c); fvm_runtime_destroy(rt);
}
