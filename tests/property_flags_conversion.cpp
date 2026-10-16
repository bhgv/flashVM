/* ASSetPropFlags target/set/clear/name conversions have observable ordering and numeric narrowing. */
#include "property_flags_fixture.hpp"
#include <limits>
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char faults[] = {0, 'T', 'S', 'C', 'N'};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned type = 0; type < 3; ++type)
    for (unsigned fault = 0; fault < 5; ++fault)
    for (unsigned exception = 0; exception < (fault ? 2U : 1U); ++exception)
    for (unsigned route = 0; route < 5; ++route) {
        if (route >= 3 && !fvm_jit_available()) continue;
        FlagFixture f; flag_init(rt, f, version, type);
        f.fault = faults[fault]; f.exception = exception != 0;
        FVM_Context *c = f.context; FVM_Value out;
        FVM_Result expected = !fault ? FVM_OK : exception ? FVM_EXCEPTION : FVM_ERROR;
        if (!route) assert(fvm_call(c, "ASSetPropFlags", 4, f.args, &out) == expected);
        else if (route == 1) { FVM_Value function; assert(fvm_get_global(c, "ASSetPropFlags", &function) == FVM_OK && fvm_function_invoke(c, function.object, 0, 4, f.args, &out) == expected); }
        else { Program code; code.variable("flagClear"); code.variable("flagSet"); code.variable("flagNames"); code.variable("flagInput"); code.number(4); code.text("ASSetPropFlags"); code.op(0x3d); code.finish(); out = boxing_run(c, code, route - 2, expected); }
        unsigned count = fault ? fault : 4;
        assert(f.used == count && !std::memcmp(f.trace, "TSCN", count));
        flag_check(c, f.target, fault ? 0U : 7U);
        if (expected == FVM_OK) assert(out.type == FVM_VALUE_UNDEFINED);
        if (expected == FVM_EXCEPTION && route < 2) assert(c->exception_pending && c->exception_value.object == f.target && fvm_exception_clear(c) == FVM_OK);
        assert(!c->gc_frames && !c->gc_native_depth && !c->exception_pending && fvm_gc_collect(c, 0) == FVM_OK);
        fvm_context_destroy(c);
    }
    const double numbers[] = {-1.0, -0.0, 0.9, 1.9, 3.9, 7.9, 8.0, 65536.0, std::numeric_limits<double>::quiet_NaN(), -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *object = 0; assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK);
        FVM_Value args[4], value, out; fvm_value_number(&value, 1); assert(fvm_object_set(c, object, "x", &value) == FVM_OK);
        args[0] = class_value(object); fvm_value_string(args + 1, "x");
        for (unsigned clear = 0; clear < 2; ++clear)
        for (unsigned i = 0; i < sizeof(numbers) / sizeof(numbers[0]); ++i) {
            assert(fvm_object_set_property_flags(c, object, "x", 0, 7) == FVM_OK && fvm_object_set_property_flags(c, object, "x", clear ? 7 : 0, 0) == FVM_OK);
            fvm_value_number(args + 2, clear ? 0 : numbers[i]); fvm_value_number(args + 3, clear ? numbers[i] : 0);
            bool valid = version ? i != 6 && i != 7 && i != 10 : numbers[i] == 0;
            FVM_Result expected = valid ? FVM_OK : FVM_INVALID_ARGUMENT;
            assert(fvm_call(c, "ASSetPropFlags", 4, args, &out) == expected);
            unsigned bits = i == 3 ? 1U : i == 4 ? 3U : i == 5 ? 7U : 0U, actual = 99;
            assert(fvm_object_property_flags(c, object, "x", &actual) == FVM_OK && actual == (valid ? clear ? 7U & ~bits : bits : clear ? 7U : 0U));
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
