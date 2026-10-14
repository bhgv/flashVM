#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include <cmath>
#include <limits>
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const double inputs[] = {1.75,-1.75,2147483647.0,2147483647.5,-2147483648.0,-2147483648.5,4294967296.0,std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity()};
    const double projected[] = {1,-1,2147483647.0,-2147483648.0,-2147483648.0,-2147483648.0,-2147483648.0,-2147483648.0,-2147483648.0,-2147483648.0};
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        for (unsigned i = 0; i < 10; ++i) {
            for (unsigned component = 0; component < 3; ++component) {
                FVM_Value initial, args[3], out; FVM_Object *date = 0;
                fvm_value_number(&initial, 0); assert(fvm_construct(c, "Date", 1, &initial, &date) == FVM_OK);
                for (unsigned k = 0; k < 3; ++k) fvm_value_number(args + k, 0);
                fvm_value_number(args + component, inputs[i]);
                assert(fvm_object_call(c, date, "setUTCMinutes", 3, args, &out) == FVM_OK);
                const double multiplier[] = {60000,1000,1};
                assert(out.number == projected[i] * multiplier[component]);
                FVM_Value stored; assert(fvm_object_call(c, date, "getTime", 0, 0, &stored) == FVM_OK && stored.number == out.number);
            }
        }
        FVM_Value initial, arg, out; FVM_Object *date = 0;
        fvm_value_number(&initial, 5.5); assert(fvm_construct(c, "Date", 1, &initial, &date) == FVM_OK);
        fvm_value_number(&arg, 0); assert(fvm_object_call(c, date, "setUTCMinutes", 1, &arg, &out) == FVM_OK && out.number == 5);
        assert(fvm_construct(c, "Date", 1, &initial, &date) == FVM_OK);
        assert(fvm_object_call(c, date, "setUTCMinutes", 0, 0, &out) == FVM_OK && out.number == -2147483648.0 * 60000 + 5);
        assert(fvm_construct(c, "Date", 1, &initial, &date) == FVM_OK);
        fvm_value_number(&arg, std::numeric_limits<double>::quiet_NaN());
        assert(fvm_object_call(c, date, "setMinutes", 1, &arg, &out) == FVM_OK);
        FVM_Value expected; FVM_Object *baseline = 0; assert(fvm_construct(c, "Date", 1, &initial, &baseline) == FVM_OK);
        fvm_value_number(&arg, -2147483648.0); assert(fvm_object_call(c, baseline, "setMinutes", 1, &arg, &expected) == FVM_OK && out.number == expected.number);
        assert(!c->gc_frames && !c->primitive_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
