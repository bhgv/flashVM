#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include <cmath>
#include <limits>
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *methods[] = {"setMonth", "setUTCMonth"};
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) {
            unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
            swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        }
        assert(fvm_standard_install(c) == FVM_OK);
        for (unsigned method = 0; method < 2; ++method) {
            FVM_Value initial, zero, expected, out, args[2]; FVM_Object *baseline = 0;
            fvm_value_number(&initial, 1592224496789.0); fvm_value_number(&zero, 0);
            assert(fvm_construct(c, "Date", 1, &initial, &baseline) == FVM_OK);
            assert(fvm_object_call(c, baseline, methods[method], 1, &zero, &expected) == FVM_OK);
            for (unsigned mode = 0; mode < 5; ++mode) {
                FVM_Object *date = 0;
                assert(fvm_construct(c, "Date", 1, &initial, &date) == FVM_OK);
                fvm_value_number(args, std::numeric_limits<double>::quiet_NaN());
                unsigned count = 1;
                if (mode == 1) count = 0;
                if (mode == 2) fvm_value_undefined(args);
                if (mode == 3) fvm_value_number(args, std::numeric_limits<double>::infinity());
                if (mode == 4) { count = 2; fvm_value_number(args, 0); fvm_value_number(args + 1, std::numeric_limits<double>::quiet_NaN()); }
                assert(fvm_object_call(c, date, methods[method], count, count ? args : 0, &out) == FVM_OK);
                if (version && mode < 3) assert(out.number == expected.number);
                else assert(out.number != out.number);
                FVM_Value stored;
                assert(fvm_object_call(c, date, "getTime", 0, 0, &stored) == FVM_OK);
                assert((stored.number != stored.number && out.number != out.number) || stored.number == out.number);
            }
        }
        assert(!c->gc_frames && !c->primitive_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
