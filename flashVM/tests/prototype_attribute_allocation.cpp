/* Per-method staging retains host attributes across failed first-load attempts. */
#define main native_instance_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "prototype_attribute_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned type = 0; type < 4; ++type)
    for (unsigned kind = 1; kind < 4; ++kind) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 3000 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
            FVM_Object *proto = prototype_target(c, type); prototype_edit(c, proto, prototype_anchors[type], kind); static_reads = 0;
            const unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; allowance = boundary;
            FVM_Result status = fvm_load_memory(c, swf, sizeof(swf)); allowance = -1;
            if (status == FVM_OUT_OF_MEMORY) { failed = true; assert(!c->swf_loaded); math_flag(c, proto, prototype_peers[type], 1); assert(fvm_gc_collect(c, 0) == FVM_OK); static_load(c, 8); }
            else { assert(status == FVM_OK); success = true; }
            math_flag(c, proto, prototype_peers[type], 3); math_flag(c, proto, prototype_anchors[type], prototype_expected(kind)); assert(!static_reads && !c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
