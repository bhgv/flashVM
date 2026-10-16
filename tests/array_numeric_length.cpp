/* Number assignment preserves raw length while Int32 controls truncation, flags and graph transfer. */
#include "property_version_fixture.hpp"
#include <limits>
static void numeric_length_equal(const FVM_Value &actual, const FVM_Value &expected)
{
    assert(actual.type == FVM_VALUE_NUMBER);
    if (expected.number != expected.number) assert(actual.number != actual.number);
    else { assert(actual.number == expected.number); if (!expected.number) assert(!std::memcmp(&actual.number, &expected.number, sizeof(double))); }
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const double values[] = {-3, -3.9, -0.0, 0.9, 1.9, 2.9, 4294967295.0, 4294967297.9,
        -4294967296.0, -4294967295.0, -2147483649.0, std::numeric_limits<double>::max(),
        std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()};
    const long bounds[] = {-3, -3, 0, 0, 1, 2, -1, 1, 0, 1, 2147483647L, 0, 0, 0, 0};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned i = 0; i < sizeof(values)/sizeof(values[0]); ++i)
    for (unsigned fixed = 0; fixed < 2; ++fixed)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value args[3], input, out; for (unsigned j = 0; j < 3; ++j) fvm_value_number(args + j, 10 + j);
        FVM_Object *array = 0, *meta = 0; assert(fvm_construct(c, "Array", 3, args, &array) == FVM_OK && fvm_object_create(c, &meta) == FVM_OK);
        FVM_Value target = class_value(array); assert(fvm_set_global(c, "numericLengthTarget", &target) == FVM_OK && fvm_object_set(c, meta, "cycle", &target) == FVM_OK);
        assert(fvm_object_set_property_flags(c, array, "2", 2, 0) == FVM_OK);
        version_flags(c, array, "length", fixed ? 0x1007 : 0x1003, 65535);
        fvm_value_number(&input, values[i]); input.metatable = meta;
        if (!route) assert(fvm_object_set(c, array, "length", &input) == FVM_OK);
        else {
            assert(fvm_set_global(c, "numericLengthInput", &input) == FVM_OK);
            Program code; code.variable("numericLengthTarget"); code.text("length"); code.variable("numericLengthInput"); code.op(0x4f); code.number(1); code.finish();
            out = boxing_run(c, code, route - 1, FVM_OK);
        }
        unsigned flags = 0; assert(fvm_object_property_flags(c, array, "length", &flags) == FVM_OK && flags == (fixed ? 0x1007 : 3));
        version_flags(c, array, "length", 0, 0xfff8);
        assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, array, "length", &out) == FVM_OK);
        FVM_Value expected; fvm_value_number(&expected, fixed ? 3 : values[i]); numeric_length_equal(out, expected);
        assert(out.metatable == (fixed ? 0 : meta));
        for (unsigned j = 0; j < 3; ++j) { char key[2] = {(char)('0' + j), 0}; int own = 0; assert(fvm_object_has_own(c, array, key, &own) == FVM_OK && !!own == (j == 2 || (long)j < bounds[i])); }
        unsigned char *data = 0; size_t bytes = 0; assert(fvm_value_serialize(c, &target, &data, &bytes) == FVM_OK);
        FVM_Context *other = 0; assert(fvm_context_create(rt, &other) == FVM_OK);
        assert(fvm_value_deserialize(other, data, bytes, &out) == FVM_OK); fvm_serialized_free(data);
        FVM_Object *copy = out.object; assert(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
        assert(fvm_object_get(other, copy, "length", &out) == FVM_OK); numeric_length_equal(out, expected);
        if (!fixed) { assert(out.metatable && fvm_object_get(other, out.metatable, "cycle", &out) == FVM_OK && out.object == copy); }
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner); fvm_context_destroy(other); fvm_context_destroy(c);
    }
    /* Unloaded embedding keeps its existing Uint32/integer setter policy. */
    for (unsigned i = 0; i < sizeof(values)/sizeof(values[0]); ++i) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Value initial[3], input, out; for (unsigned j = 0; j < 3; ++j) fvm_value_number(initial + j, j + 10);
        FVM_Object *array = 0; assert(fvm_construct(c, "Array", 3, initial, &array) == FVM_OK);
        fvm_value_number(&input, values[i]);
        const bool valid = values[i] >= 0 && values[i] <= 4294967295.0 && values[i] == std::floor(values[i]);
        assert(fvm_object_set(c, array, "length", &input) == (valid ? FVM_OK : FVM_INVALID_ARGUMENT));
        assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == (valid ? values[i] : 3) && !out.metatable);
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
