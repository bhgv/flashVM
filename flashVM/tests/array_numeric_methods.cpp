/* Numeric raw length is preserved by readers and normalized only when a method writes length. */
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"
#include <limits>
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const double raw[] = {-3.9, -0.0, 0.9, 1.9, 2.9, 4294967297.9, 4294967295.0,
        std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::max()};
    const long bounds[] = {-3, 0, 0, 1, 2, 1, -1, 0, 0, 0};
    const char *methods[] = {"join", "sort", "reverse", "slice", "concat", "push", "pop", "shift", "unshift", "splice"};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned input = 0; input < sizeof(raw)/sizeof(raw[0]); ++input)
    for (unsigned method = 0; method < 10; ++method)
    for (unsigned fixed = 0; fixed < 2; ++fixed) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = 0, *meta = 0; assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK);
        FVM_Value value = class_value(array), out, argument; assert(fvm_set_global(c, "rawLengthRoot", &value) == FVM_OK);
        fvm_value_number(&value, raw[input]); value.metatable = meta;
        assert(fvm_object_set(c, array, "length", &value) == FVM_OK);
        for (unsigned i = 0; i < 3; ++i) { fvm_value_number(&value, (i + 1) * 11); assert(fvm_array_store_own(c, array, i, value) == FVM_OK); }
        if (fixed) assert(fvm_object_set_property_flags(c, array, "length", 4, 0) == FVM_OK);
        fvm_value_number(&argument, method == 1 ? 16 : 0);
        assert(fvm_object_call(c, array, methods[method], method == 1 || method == 9 ? 1 : 0, &argument, &out) == FVM_OK);
        if (!method) {
            const char *want = bounds[input] > 1 ? "11,22" : bounds[input] == 1 ? "11" : "";
            assert(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, want));
        }
        if (method == 1 || method == 2) assert(out.type == FVM_VALUE_OBJECT && out.object == array);
        if (method == 3 || method == 4 || method == 9) {
            assert(out.type == FVM_VALUE_OBJECT);
            assert(fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == (bounds[input] > 0 ? bounds[input] : 0));
        }
        const bool writes = !fixed && (method == 5 || method == 8 || method == 9 || ((method == 6 || method == 7) && bounds[input]));
        double expected = raw[input];
        if (writes) expected = method == 9 && bounds[input] > 0 ? 0 : (method == 6 || method == 7) ? bounds[input] - 1 : bounds[input];
        assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, array, "length", &value) == FVM_OK && value.type == FVM_VALUE_NUMBER);
        if (expected != expected) assert(value.number != value.number); else assert(value.number == expected);
        if (!expected && !writes) assert(!std::memcmp(&value.number, raw + input, sizeof(double)));
        assert(value.metatable == (writes ? 0 : meta));
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
