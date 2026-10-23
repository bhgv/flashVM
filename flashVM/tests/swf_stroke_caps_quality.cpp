#include "swf_stroke_caps_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    for(unsigned type=0;type<4;++type) for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) for(unsigned q=0;q<4;++q) {
        assert(fvm_stage_set_quality(c,(FVM_StageQuality)q)==FVM_OK);
        FVM_Object *root=load(c,bs_tags(caps_shape(type,cap,join,80,true,true)),1); tick(c,root);
        caps_pixels(c,root,type,cap,join,80,true,true,q!=FVM_STAGE_QUALITY_LOW);
        assert(fvm_gc_unpin(c,root)==FVM_OK); assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
