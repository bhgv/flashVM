#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const double years[] = {-2000,-1901,-1900,-1,0,99,100};
    const double expected[] = {-100,-1,0,1899,1900,1999,100};
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) {
            unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value klass; assert(fvm_get_global(c, "Date", &klass) == FVM_OK);
        for (unsigned i = 0; i < 7; ++i) {
            FVM_Value args[2], timestamp, out; FVM_Object *date = 0;
            fvm_value_number(args, years[i]); fvm_value_number(args + 1, 0);
            double year = !version && years[i] < 0 ? years[i] : expected[i];
            assert(fvm_object_call(c, klass.object, "UTC", 2, args, &timestamp) == FVM_OK);
            if (version && years[i] == -1) assert(timestamp.number == -2240524800000.0);
            assert(fvm_construct(c, "Date", 1, &timestamp, &date) == FVM_OK);
            assert(fvm_object_call(c, date, "getUTCFullYear", 0, 0, &out) == FVM_OK && out.number == year);
            assert(fvm_construct(c, "Date", 2, args, &date) == FVM_OK);
            assert(fvm_object_call(c, date, "getFullYear", 0, 0, &out) == FVM_OK && out.number == year);
        }
        assert(!c->gc_frames && !c->primitive_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
