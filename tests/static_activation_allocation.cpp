/* First-load OOM preserves class flags even when its prototype cannot activate. */
#define main native_instance_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "static_activation_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned type = 0; type < 2; ++type)
    for (unsigned variant = 1; variant < 7; ++variant) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 3000 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
            FVM_Object *klass = static_class(c, type); static_prototype(c, klass, variant); static_reads = 0;
            const unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; allowance = boundary;
            FVM_Result status = fvm_load_memory(c, swf, sizeof(swf)); allowance = -1;
            if (status == FVM_OUT_OF_MEMORY) { failed = true; assert(!c->swf_loaded); static_flags(c, klass, type, type ? 1U : 0U); assert(fvm_gc_collect(c, 0) == FVM_OK); static_load(c, 8); }
            else { assert(status == FVM_OK); success = true; }
            static_flags(c, klass, type, 7); assert(!static_reads && !c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
