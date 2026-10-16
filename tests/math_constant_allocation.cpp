/* A failed first SWF load retains host Math numbers and attributes before retry. */
#define main native_instance_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "math_constant_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; version += 2) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 3000 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
            FVM_Value math; assert(fvm_get_global(c, "Math", &math) == FVM_OK);
            unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
            FVM_Value originals[8];
            for (unsigned i = 0; i < 8; ++i) assert(fvm_object_get(c, math.object, math_fields[i], originals + i) == FVM_OK);
            allowance = boundary;
            FVM_Result status = fvm_load_memory(c, swf, sizeof(swf));
            allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth);
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; assert(!c->swf_loaded);
                for (unsigned i = 0; i < sizeof(math_fields) / sizeof(math_fields[0]); ++i) math_flag(c, math.object, math_fields[i], i < 8 ? 0U : 1U);
                for (unsigned i = 0; i < 8; ++i) { FVM_Value value; assert(fvm_object_get(c, math.object, math_fields[i], &value) == FVM_OK && value.number == originals[i].number); }
                assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
            }
            else { assert(status == FVM_OK); success = true; }
            for (unsigned i = 0; i < sizeof(math_fields) / sizeof(math_fields[0]); ++i) math_flag(c, math.object, math_fields[i], 7);
            math_constant_namespace(c, math.object);
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
