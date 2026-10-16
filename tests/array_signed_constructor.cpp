/* Numeric Array construction uses clamped Int32, unlike subsequent raw length assignment. */
#include "property_version_fixture.hpp"
#include <limits>
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const double inputs[] = {0, -0.0, 0.9, -0.9, 2.9, -2.9, 2147483647.0,
        2147483647.5, 2147483648.0, 4294967295.0, 4294967296.0,
        -2147483648.0, -2147483648.5, -4294967296.0,
        std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN()};
    const double expected[] = {0, 0, 0, 0, 2, -2, 2147483647.0,
        -2147483648.0, -2147483648.0, -2147483648.0, -2147483648.0,
        -2147483648.0, -2147483648.0, -2147483648.0, -2147483648.0, -2147483648.0, -2147483648.0};
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        for (unsigned route = 0; route < 8; ++route) {
            if ((route == 3 || route == 4 || route == 6 || route == 7) && !fvm_jit_available()) continue;
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
            if (version) version_load(c, version);
            assert(fvm_standard_install(c) == FVM_OK);
            for (unsigned i = 0; i < sizeof(inputs) / sizeof(inputs[0]); ++i) {
                FVM_Value input, out, length; fvm_value_number(&input, inputs[i]);
                const bool host_valid = inputs[i] >= 0 && inputs[i] <= 4294967295.0 && inputs[i] == std::floor(inputs[i]);
                const FVM_Result status = !version && !host_valid ? FVM_INVALID_ARGUMENT : FVM_OK;
                if (!route) {
                    FVM_Object *array = 0;
                    assert(fvm_construct(c, "Array", 1, &input, &array) == status);
                    out = class_value(array);
                } else if (route == 1) assert(fvm_call(c, "Array", 1, &input, &out) == status);
                else {
                    assert(fvm_set_global(c, "signedArrayInput", &input) == FVM_OK);
                    Program code; code.variable("signedArrayInput"); code.number(1); code.text("Array");
                    code.op(route < 5 ? 0x40 : 0x3d); code.finish();
                    out = boxing_run(c, code, route < 5 ? route - 2 : route - 5, status);
                }
                if (status != FVM_OK) continue;
                assert(out.type == FVM_VALUE_OBJECT && out.object && out.object->resource_kind == FVM_RESOURCE_ARRAY);
                assert(fvm_object_get(c, out.object, "length", &length) == FVM_OK && length.type == FVM_VALUE_NUMBER);
                const double want = version ? expected[i] : inputs[i];
                assert(length.number == want && (!version || want || !std::signbit(length.number)));
                assert(out.object->indexed.empty());
                assert(fvm_set_global(c, "signedArrayRoot", &out) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
                if (version && !route) {
                    unsigned char *bytes = 0; size_t count = 0;
                    assert(fvm_object_set(c, out.object, "cycle", &out) == FVM_OK);
                    assert(fvm_value_serialize(c, &out, &bytes, &count) == FVM_OK);
                    FVM_Context *other = 0; assert(fvm_context_create(rt, &other) == FVM_OK);
                    version_load(other, version);
                    assert(fvm_value_deserialize(other, bytes, count, &out) == FVM_OK); fvm_serialized_free(bytes);
                    assert(fvm_gc_pin(other, out.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
                    assert(fvm_object_get(other, out.object, "length", &length) == FVM_OK && length.number == want);
                    FVM_Value cycle; assert(fvm_object_get(other, out.object, "cycle", &cycle) == FVM_OK && cycle.object == out.object);
                    fvm_context_destroy(other);
                }
            }
            assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner);
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
