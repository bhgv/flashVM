#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include <cmath>
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const double inputs[] = {-0.25,-0.75,-1,-86400001,-3600000.25,5.5,86400005.5};
    const char *getters[] = {"getUTCFullYear","getUTCMonth","getUTCDate","getFullYear","getMonth","getDate"};
    const char *setters[] = {"setUTCFullYear","setUTCMonth","setUTCDate","setFullYear","setMonth","setDate"};
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        for (unsigned i = 0; i < 7; ++i) for (unsigned method = 0; method < 6; ++method) {
            FVM_Value initial, component, offset, out; FVM_Object *date = 0;
            fvm_value_number(&initial, inputs[i]); assert(fvm_construct(c, "Date", 1, &initial, &date) == FVM_OK);
            assert(fvm_object_call(c, date, getters[method], 0, 0, &component) == FVM_OK);
            assert(fvm_object_call(c, date, "getTimezoneOffset", 0, 0, &offset) == FVM_OK);
            double local = inputs[i] - (method >= 3 ? offset.number*60000 : 0);
            double expected = std::floor(inputs[i]);
            if (version <= 7 && std::fmod(local,86400000) < 0) expected -= 86400000;
            assert(fvm_object_call(c, date, setters[method], 1, &component, &out) == FVM_OK && out.number == expected);
            assert(fvm_object_call(c, date, "getTime", 0, 0, &out) == FVM_OK && out.number == expected);
        }
        assert(!c->gc_frames && !c->primitive_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
