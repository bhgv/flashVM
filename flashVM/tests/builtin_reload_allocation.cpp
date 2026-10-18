/* A failed replacement load retains the previous SWF and all edited builtin attributes. */
#define main native_instance_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "builtin_reload_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned source = 4; source <= 8; ++source)
    for (unsigned destination = 4; destination <= 8; ++destination) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 1000 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); reload_load(c, source); assert(fvm_standard_install(c) == FVM_OK);
            FVM_Object *targets[10];
            for (unsigned i = 0; i < 10; ++i) { targets[i] = reload_target(c, i); assert(fvm_object_set_property_flags(c, targets[i], reload_fields[i], 0, 7) == FVM_OK && fvm_object_set_property_flags(c, targets[i], reload_fields[i], 1, 0) == FVM_OK); }
            unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)destination;
            allowance = boundary;
            FVM_Result status = fvm_load_memory(c, swf, sizeof(swf));
            allowance = -1;
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; assert(c->swf_loaded && c->swf_info.version == source && c->swf_bytes[3] == source);
                for (unsigned i = 0; i < 10; ++i) reload_flags(c, targets[i], reload_fields[i], 1);
                assert(fvm_gc_collect(c, 0) == FVM_OK); reload_load(c, destination);
            }
            else { assert(status == FVM_OK); success = true; }
            assert(c->swf_info.version == destination && c->swf_bytes[3] == destination);
            for (unsigned i = 0; i < 10; ++i) { assert(reload_target(c, i) == targets[i]); reload_flags(c, targets[i], reload_fields[i], 1); }
            assert(!c->gc_frames && !c->gc_native_depth);
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
