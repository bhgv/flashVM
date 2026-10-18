/* Numeric length truncation and graph restoration preserve call/root state at every allocation boundary. */
#define main numeric_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "property_version_fixture.hpp"
#include <limits>
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const double values[] = {-2.9, std::numeric_limits<double>::quiet_NaN(), 4294967297.9};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 3; ++kind) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 512 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
            FVM_Value args[3], value, out; for (unsigned i = 0; i < 3; ++i) fvm_value_number(args + i, i + 11);
            FVM_Object *array = 0, *meta = 0; assert(fvm_construct(c, "Array", 3, args, &array) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK);
            assert(fvm_gc_pin(c, array) == FVM_OK && fvm_gc_pin(c, meta) == FVM_OK);
            value = class_value(array); assert(fvm_object_set(c, meta, "cycle", &value) == FVM_OK);
            fvm_value_number(&value, values[kind]); value.metatable = meta;
            allowance = boundary; FVM_Result status = fvm_object_set(c, array, "length", &value); allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner);
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_set(c, array, "length", &value) == FVM_OK);
            } else { assert(status == FVM_OK); success = true; }
            assert(fvm_gc_unpin(c, meta) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.metatable == meta);
            if (kind == 1) assert(out.number != out.number); else assert(out.number == values[kind]);
            assert(fvm_object_get(c, out.metatable, "cycle", &out) == FVM_OK && out.object == array);
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    FVM_Context *source = 0; assert(fvm_context_create(rt, &source) == FVM_OK); version_load(source, 8); assert(fvm_standard_install(source) == FVM_OK);
    FVM_Object *array = 0; assert(fvm_construct(source, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(source, array) == FVM_OK);
    FVM_Value value; fvm_value_number(&value, std::numeric_limits<double>::quiet_NaN()); value.metatable = array;
    assert(fvm_object_set(source, array, "length", &value) == FVM_OK); value = class_value(array);
    unsigned char *data = 0; size_t bytes = 0; assert(fvm_value_serialize(source, &value, &data, &bytes) == FVM_OK);
    bool failed = false, success = false;
    for (long boundary = 0; boundary < 4096 && !success; ++boundary) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); unsigned long before = 0, after = 0; assert(fvm_gc_object_count(c, &before) == FVM_OK);
        allowance = boundary; FVM_Result status = fvm_value_deserialize(c, data, bytes, &value); allowance = -1;
        if (status == FVM_OUT_OF_MEMORY) {
            failed = true; assert(value.type == FVM_VALUE_UNDEFINED && fvm_gc_object_count(c, &after) == FVM_OK && before == after);
            assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_value_deserialize(c, data, bytes, &value) == FVM_OK);
        } else { assert(status == FVM_OK); success = true; }
        FVM_Value length; assert(fvm_gc_pin(c, value.object) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        assert(fvm_object_get(c, value.object, "length", &length) == FVM_OK && length.number != length.number && length.metatable == value.object);
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    assert(failed && success); fvm_serialized_free(data); fvm_context_destroy(source); fvm_runtime_destroy(rt);
}
