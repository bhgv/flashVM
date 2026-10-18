#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include <cmath>
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const double inputs[] = {-0.25,-0.75,3599999.75,86399999.75,-3600000.25};
    const double hours[] = {0,23,1,0,23};
    const double ms[] = {0,0,999,999,0};
    const char *setters[] = {"setUTCHours","setUTCMinutes","setUTCSeconds","setUTCMilliseconds"};
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        for (unsigned i = 0; i < 5; ++i) {
            double minute = std::fmod(std::floor(inputs[i]/60000),60); if (minute < 0) minute += 60;
            double second = std::fmod(std::floor(inputs[i]/1000),60); if (second < 0) second += 60;
            for (unsigned method = 0; method < 4; ++method) {
                FVM_Value initial, zero, out; FVM_Object *date = 0;
                fvm_value_number(&initial, inputs[i]); fvm_value_number(&zero, 0);
                assert(fvm_construct(c, "Date", 1, &initial, &date) == FVM_OK);
                double components[] = {hours[i],minute,second,ms[i]}; components[method] = 0;
                double expected = std::floor(inputs[i]/86400000)*86400000 + components[0]*3600000 + components[1]*60000 + components[2]*1000 + components[3];
                assert(fvm_object_call(c, date, setters[method], 1, &zero, &out) == FVM_OK && out.number == expected);
                FVM_Value stored; assert(fvm_object_call(c, date, "getTime", 0, 0, &stored) == FVM_OK && stored.number == expected);
            }
        }
        assert(!c->gc_frames && !c->primitive_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
